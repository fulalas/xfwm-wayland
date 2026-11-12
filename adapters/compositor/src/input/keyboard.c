#include <linux/input-event-codes.h>
#include <stdlib.h>
#include <wlr/backend/session.h>
#include <wlr/interfaces/wlr_keyboard.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_seat.h>
#include <xkbcommon/xkbcommon.h>

#include "input_mapping.h"
#include "keymap_cache.h"
#include "osd/osd.h"
#include "util/listener.h"
#include "seat_private.h"
#include "xwayland/xwayland.h"
#include "xwayland/xwayland_keys.h"

enum {
    XKB_KEYCODE_OFFSET = 8,
    DEFAULT_REPEAT_RATE = 25,
    DEFAULT_REPEAT_DELAY_MS = 600,
    HELD_KEYS_MAX = 32,
};

typedef struct key_list {
    uint32_t keys[HELD_KEYS_MAX];
    size_t count;
} key_list;

typedef struct keyboard {
    keyboards *owner;
    struct wlr_keyboard *wlr_keyboard;
    bool follows_prefs;
    key_list consumed;
    key_list shared;
    uint32_t pressed_keysyms[KEY_CNT];
    bool x11_modifiers_shared;
    struct wl_list link;
    struct wl_listener key;
    struct wl_listener modifiers;
    struct wl_listener destroy;
} keyboard;

struct keyboards {
    seat *seat;
    keymap_cache *keymaps;
    bool numlock_pref;
    struct wl_list list;
};

static keyboard *keyboard_create(keyboards *owner, struct wlr_keyboard *wlr_keyboard);
static void keyboard_destroy(keyboard *keyboard);
static keyboard_prefs current_prefs(const keyboards *keyboards);
static bool keyboard_apply_prefs(keyboard *keyboard, const keyboard_prefs *prefs);
static void lock_numlock(struct wlr_keyboard *keyboard);
static void handle_key(struct wl_listener *listener, void *data);
static void handle_modifiers(struct wl_listener *listener, void *data);
static void handle_destroy(struct wl_listener *listener, void *data);
static void report_modifiers(keyboards *keyboards);
static uint32_t combined_modifiers(const keyboards *keyboards);
static bool switch_vt(keyboard *keyboard, const struct wlr_keyboard_key_event *event,
                      const key_chord *chord);
static wm_dispatch dispatch_key(keyboard *keyboard, const struct wlr_keyboard_key_event *event,
                                const key_chord *chord);
static bool core_receives_keys(const seat *seat);
static uint32_t core_keysym(keyboard *keyboard, const struct wlr_keyboard_key_event *event,
                            const key_chord *chord);
static uint32_t bound_keysym(const wm_desktop *desktop, const key_chord *chord);
static uint32_t base_keysym(struct wlr_keyboard *keyboard, uint32_t keycode);
static key_chord chord_for(struct wlr_keyboard *keyboard, uint32_t keycode);
static bool key_list_take(key_list *list, uint32_t keycode);
static bool key_list_has(const key_list *list, uint32_t keycode);
static void key_list_add(key_list *list, uint32_t keycode);
static void hide_held_keys(keyboard *keyboard);
static void forward_key(keyboard *keyboard, const struct wlr_keyboard_key_event *event);
static void share_with_x11(keyboard *keyboard, const struct wlr_keyboard_key_event *event,
                           const key_chord *chord);
static bool send_to_x11(keyboard *keyboard, const struct wlr_keyboard_key_event *event);
static void share_modifiers_with_x11(keyboard *keyboard);
static void release_shared_keys(keyboard *keyboard);
static xwayland_keys_route x11_route(const keyboard *keyboard);

keyboards *keyboards_create(seat *s)
{
    keyboards *kbs = calloc(1, sizeof(*kbs));
    if (kbs == NULL) {
        return NULL;
    }
    kbs->seat = s;
    wl_list_init(&kbs->list);
    kbs->keymaps = keymap_cache_create();
    if (kbs->keymaps == NULL) {
        free(kbs);
        return NULL;
    }
    return kbs;
}

void keyboards_destroy(keyboards *kbs)
{
    keyboard *kb;
    keyboard *tmp;
    wl_list_for_each_safe(kb, tmp, &kbs->list, link) {
        keyboard_destroy(kb);
    }
    keymap_cache_destroy(kbs->keymaps);
    free(kbs);
}

void keyboards_add(keyboards *kbs, struct wlr_keyboard *wlr_keyboard)
{
    keyboard *kb = keyboard_create(kbs, wlr_keyboard);
    if (kb == NULL) {
        return;
    }
    kb->follows_prefs = true;
    keyboard_prefs prefs = current_prefs(kbs);
    keymap_cache_update(kbs->keymaps, &prefs);
    keyboard_apply_prefs(kb, &prefs);
    if (prefs.numlock) {
        lock_numlock(wlr_keyboard);
    }
    kbs->numlock_pref = prefs.numlock;
    wlr_seat_set_keyboard(kbs->seat->wlr_seat, wlr_keyboard);
}

void keyboards_add_virtual(keyboards *kbs, struct wlr_keyboard *wlr_keyboard)
{
    if (keyboard_create(kbs, wlr_keyboard) != NULL) {
        wlr_seat_set_keyboard(kbs->seat->wlr_seat, wlr_keyboard);
    }
}

void keyboards_reload(keyboards *kbs)
{
    keyboard_prefs prefs = current_prefs(kbs);
    bool numlock_turned_on = prefs.numlock && !kbs->numlock_pref;
    keymap_cache_update(kbs->keymaps, &prefs);
    keyboard *kb;
    wl_list_for_each(kb, &kbs->list, link) {
        if (!kb->follows_prefs) {
            continue;
        }
        bool keymap_changed = keyboard_apply_prefs(kb, &prefs);
        if (numlock_turned_on || (prefs.numlock && keymap_changed)) {
            lock_numlock(kb->wlr_keyboard);
        }
    }
    kbs->numlock_pref = prefs.numlock;
}

size_t keyboards_forwarded_keys(const keyboards *kbs, const struct wlr_keyboard *wlr_keyboard,
                                uint32_t *keys)
{
    const keyboard *owner = NULL;
    const keyboard *kb;
    wl_list_for_each(kb, &kbs->list, link) {
        if (kb->wlr_keyboard == wlr_keyboard) {
            owner = kb;
        }
    }
    size_t count = 0;
    for (size_t i = 0; i < wlr_keyboard->num_keycodes; i++) {
        uint32_t keycode = wlr_keyboard->keycodes[i];
        if (owner == NULL || !key_list_has(&owner->consumed, keycode)) {
            keys[count++] = keycode;
        }
    }
    return count;
}

static keyboard *keyboard_create(keyboards *owner, struct wlr_keyboard *wlr_keyboard)
{
    keyboard *kb = calloc(1, sizeof(*kb));
    if (kb == NULL) {
        return NULL;
    }
    kb->owner = owner;
    kb->wlr_keyboard = wlr_keyboard;
    listener_connect(&kb->key, &wlr_keyboard->events.key, handle_key);
    listener_connect(&kb->modifiers, &wlr_keyboard->events.modifiers, handle_modifiers);
    listener_connect(&kb->destroy, &wlr_keyboard->base.events.destroy, handle_destroy);
    wl_list_insert(&owner->list, &kb->link);
    return kb;
}

static void keyboard_destroy(keyboard *kb)
{
    release_shared_keys(kb);
    listener_disconnect(&kb->key);
    listener_disconnect(&kb->modifiers);
    listener_disconnect(&kb->destroy);
    wl_list_remove(&kb->link);
    free(kb);
}

static keyboard_prefs current_prefs(const keyboards *kbs)
{
    const compositor_services *services = kbs->seat->server->services;
    return services->keyboard_prefs(services->self);
}

static bool keyboard_apply_prefs(keyboard *kb, const keyboard_prefs *prefs)
{
    struct xkb_keymap *keymap = keymap_cache_keymap(kb->owner->keymaps);
    bool keymap_changed = keymap != NULL && kb->wlr_keyboard->keymap != keymap &&
                          wlr_keyboard_set_keymap(kb->wlr_keyboard, keymap);
    int rate = prefs->repeat_rate >= 0 ? prefs->repeat_rate : DEFAULT_REPEAT_RATE;
    int delay = prefs->repeat_delay_ms > 0 ? prefs->repeat_delay_ms : DEFAULT_REPEAT_DELAY_MS;
    wlr_keyboard_set_repeat_info(kb->wlr_keyboard, rate, delay);
    return keymap_changed;
}

static void lock_numlock(struct wlr_keyboard *wlr_keyboard)
{
    if (wlr_keyboard->keymap == NULL) {
        return;
    }
    xkb_mod_index_t index = xkb_keymap_mod_get_index(wlr_keyboard->keymap, XKB_MOD_NAME_NUM);
    if (index == XKB_MOD_INVALID) {
        return;
    }
    const struct wlr_keyboard_modifiers *mods = &wlr_keyboard->modifiers;
    wlr_keyboard_notify_modifiers(wlr_keyboard, mods->depressed, mods->latched,
                                  mods->locked | (1u << index), mods->group);
}

static void handle_key(struct wl_listener *listener, void *data)
{
    keyboard *kb = wl_container_of(listener, kb, key);
    const struct wlr_keyboard_key_event *event = data;
    seat_notify_activity(kb->owner->seat);
    key_chord chord = chord_for(kb->wlr_keyboard, event->keycode);
    if (switch_vt(kb, event, &chord)) {
        return;
    }
    wm_dispatch result = dispatch_key(kb, event, &chord);
    if (result == WM_FORWARD) {
        forward_key(kb, event);
    }
    if (result == WM_FORWARD || event->state != WL_KEYBOARD_KEY_STATE_PRESSED) {
        share_with_x11(kb, event, &chord);
    }
}

static void handle_modifiers(struct wl_listener *listener, void *data)
{
    (void) data;
    keyboard *kb = wl_container_of(listener, kb, modifiers);
    seat *s = kb->owner->seat;
    wlr_seat_set_keyboard(s->wlr_seat, kb->wlr_keyboard);
    wlr_seat_keyboard_notify_modifiers(s->wlr_seat, &kb->wlr_keyboard->modifiers);
    share_modifiers_with_x11(kb);
    report_modifiers(kb->owner);
}

static void handle_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    keyboard *kb = wl_container_of(listener, kb, destroy);
    keyboards *kbs = kb->owner;
    keyboard_destroy(kb);
    report_modifiers(kbs);
}

static void report_modifiers(keyboards *kbs)
{
    seat *s = kbs->seat;
    uint32_t mods = combined_modifiers(kbs);
    wm_modifiers_changed(s->server->desktop, mods);
    selection_modifiers_changed(s->selection, mods);
}

static uint32_t combined_modifiers(const keyboards *kbs)
{
    uint32_t mods = 0;
    const keyboard *kb;
    wl_list_for_each(kb, &kbs->list, link) {
        mods |= input_mapping_modifiers(wlr_keyboard_get_modifiers(kb->wlr_keyboard));
    }
    return mods;
}

static bool switch_vt(keyboard *kb, const struct wlr_keyboard_key_event *event,
                      const key_chord *chord)
{
    unsigned vt = input_mapping_vt(chord);
    struct wlr_session *session = kb->owner->seat->server->session;
    if (vt == 0 || session == NULL) {
        return false;
    }
    if (event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        wlr_session_change_vt(session, vt);
    }
    return true;
}

static wm_dispatch dispatch_key(keyboard *kb, const struct wlr_keyboard_key_event *event,
                                const key_chord *chord)
{
    seat *s = kb->owner->seat;
    bool released = event->state != WL_KEYBOARD_KEY_STATE_PRESSED;
    bool press_was_consumed = released && key_list_take(&kb->consumed, event->keycode);
    uint32_t keysym = core_keysym(kb, event, chord);
    if (!press_was_consumed && !core_receives_keys(s)) {
        return WM_FORWARD;
    }
    wm_key_event key = {
        .keysym = keysym,
        .modifiers = chord->modifiers,
        .time_ms = event->time_msec,
    };
    if (released) {
        wm_key_released(s->server->desktop, &key);
        return press_was_consumed ? WM_CONSUMED : WM_FORWARD;
    }
    struct wlr_surface *focused = s->wlr_seat->keyboard_state.focused_surface;
    size_t consumed_count = kb->consumed.count;
    key_list_add(&kb->consumed, event->keycode);
    if (chord->modifiers != 0) {
        hide_held_keys(kb);
    }
    wm_dispatch result = wm_key_pressed(s->server->desktop, &key);
    if (result != WM_CONSUMED) {
        kb->consumed.count = consumed_count;
        return result;
    }
    bool hid_keys = kb->consumed.count > consumed_count + 1;
    if (!hid_keys || s->wlr_seat->keyboard_state.focused_surface != focused) {
        return result;
    }
    if (s->server->osd != NULL && osd_is_modal(s->server->osd)) {
        wlr_seat_set_keyboard(s->wlr_seat, kb->wlr_keyboard);
        seat_focus_reenter(s);
    } else {
        kb->consumed.count = consumed_count + 1;
    }
    return result;
}

static bool core_receives_keys(const seat *s)
{
    if (seat_input_route(s) == INPUT_ROUTE_LOCKED) {
        return false;
    }
    return !shortcuts_inhibited(s->shortcuts, s->focus.current) &&
           !xwayland_shell_grabs_keyboard(s->server->xwayland, s->focus.current);
}

static uint32_t core_keysym(keyboard *kb, const struct wlr_keyboard_key_event *event,
                            const key_chord *chord)
{
    bool remembered = event->keycode < KEY_CNT;
    if (event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        uint32_t keysym = bound_keysym(kb->owner->seat->server->desktop, chord);
        if (remembered) {
            kb->pressed_keysyms[event->keycode] = keysym;
        }
        return keysym;
    }
    uint32_t pressed = remembered ? kb->pressed_keysyms[event->keycode] : XKB_KEY_NoSymbol;
    return pressed != XKB_KEY_NoSymbol ? pressed : input_mapping_keysym(chord->base_keysym);
}

static uint32_t bound_keysym(const wm_desktop *desktop, const key_chord *chord)
{
    wm_key_event key = {input_mapping_keysym(chord->base_keysym), chord->modifiers, 0};
    if (wm_key_is_bound(desktop, &key)) {
        return key.keysym;
    }
    wm_key_event level_key = {input_mapping_keysym(chord->translated_keysym), chord->modifiers, 0};
    return wm_key_is_bound(desktop, &level_key) ? level_key.keysym : key.keysym;
}

static uint32_t base_keysym(struct wlr_keyboard *wlr_keyboard, uint32_t keycode)
{
    if (wlr_keyboard->keymap == NULL || wlr_keyboard->xkb_state == NULL) {
        return XKB_KEY_NoSymbol;
    }
    xkb_keycode_t code = keycode + XKB_KEYCODE_OFFSET;
    xkb_layout_index_t layout = xkb_state_key_get_layout(wlr_keyboard->xkb_state, code);
    if (layout == XKB_LAYOUT_INVALID) {
        layout = 0;
    }
    const xkb_keysym_t *syms = NULL;
    int count = xkb_keymap_key_get_syms_by_level(wlr_keyboard->keymap, code, layout, 0, &syms);
    return count > 0 ? syms[0] : XKB_KEY_NoSymbol;
}

static key_chord chord_for(struct wlr_keyboard *wlr_keyboard, uint32_t keycode)
{
    xkb_keycode_t code = keycode + XKB_KEYCODE_OFFSET;
    struct xkb_state *state = wlr_keyboard->xkb_state;
    return (key_chord) {
        .translated_keysym = state != NULL ? xkb_state_key_get_one_sym(state, code)
                                           : XKB_KEY_NoSymbol,
        .base_keysym = base_keysym(wlr_keyboard, keycode),
        .modifiers = input_mapping_modifiers(wlr_keyboard_get_modifiers(wlr_keyboard)),
    };
}

static bool key_list_take(key_list *list, uint32_t keycode)
{
    for (size_t i = 0; i < list->count; i++) {
        if (list->keys[i] == keycode) {
            list->keys[i] = list->keys[--list->count];
            return true;
        }
    }
    return false;
}

static bool key_list_has(const key_list *list, uint32_t keycode)
{
    for (size_t i = 0; i < list->count; i++) {
        if (list->keys[i] == keycode) {
            return true;
        }
    }
    return false;
}

static void key_list_add(key_list *list, uint32_t keycode)
{
    if (list->count < HELD_KEYS_MAX) {
        list->keys[list->count++] = keycode;
    }
}

static void hide_held_keys(keyboard *kb)
{
    for (size_t i = 0; i < kb->wlr_keyboard->num_keycodes; i++) {
        uint32_t keycode = kb->wlr_keyboard->keycodes[i];
        if (!key_list_has(&kb->consumed, keycode)) {
            key_list_add(&kb->consumed, keycode);
        }
    }
}

static void forward_key(keyboard *kb, const struct wlr_keyboard_key_event *event)
{
    struct wlr_seat *wlr_seat = kb->owner->seat->wlr_seat;
    wlr_seat_set_keyboard(wlr_seat, kb->wlr_keyboard);
    wlr_seat_keyboard_notify_key(wlr_seat, event->time_msec, event->keycode, event->state);
}

static void share_with_x11(keyboard *kb, const struct wlr_keyboard_key_event *event,
                           const key_chord *chord)
{
    if (event->state != WL_KEYBOARD_KEY_STATE_PRESSED) {
        if (key_list_take(&kb->shared, event->keycode)) {
            send_to_x11(kb, event);
        }
        return;
    }
    if (kb->shared.count < HELD_KEYS_MAX && core_receives_keys(kb->owner->seat)
        && input_mapping_shared_with_x11(chord) && send_to_x11(kb, event)) {
        key_list_add(&kb->shared, event->keycode);
        kb->x11_modifiers_shared = true;
    }
}

static bool send_to_x11(keyboard *kb, const struct wlr_keyboard_key_event *event)
{
    xwayland_keys_route route = x11_route(kb);
    return xwayland_keys_send(&route, event);
}

static void share_modifiers_with_x11(keyboard *kb)
{
    if (!kb->x11_modifiers_shared) {
        return;
    }
    xwayland_keys_route route = x11_route(kb);
    xwayland_keys_send_modifiers(&route);
    const struct wlr_keyboard_modifiers *mods = &kb->wlr_keyboard->modifiers;
    kb->x11_modifiers_shared = mods->depressed != 0 || mods->latched != 0;
}

static void release_shared_keys(keyboard *kb)
{
    for (size_t i = 0; i < kb->shared.count; i++) {
        struct wlr_keyboard_key_event release = {
            .keycode = kb->shared.keys[i],
            .state = WL_KEYBOARD_KEY_STATE_RELEASED,
        };
        send_to_x11(kb, &release);
    }
    kb->shared.count = 0;
}

static xwayland_keys_route x11_route(const keyboard *kb)
{
    xwayland_shell *shell = kb->owner->seat->server->xwayland;
    return (xwayland_keys_route){kb->owner->seat->wlr_seat,
                                 shell != NULL ? xwayland_shell_client(shell) : NULL,
                                 kb->wlr_keyboard};
}
