#include <stdlib.h>
#include <wlr/backend/session.h>
#include <wlr/interfaces/wlr_keyboard.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_seat.h>
#include <xkbcommon/xkbcommon.h>

#include "input_mapping.h"
#include "keymap_cache.h"
#include "listener.h"
#include "seat_private.h"

enum {
    XKB_KEYCODE_OFFSET = 8,
    DEFAULT_REPEAT_RATE = 25,
    DEFAULT_REPEAT_DELAY_MS = 600,
    CONSUMED_KEYS_MAX = 32,
};

typedef struct keyboard {
    keyboards *owner;
    struct wlr_keyboard *wlr_keyboard;
    bool follows_prefs;
    uint32_t consumed[CONSUMED_KEYS_MAX];
    size_t consumed_count;
    struct wl_list link;
    struct wl_listener key;
    struct wl_listener modifiers;
    struct wl_listener destroy;
} keyboard;

struct keyboards {
    seat *seat;
    keymap_cache *keymaps;
    struct wl_list list;
};

static keyboard *keyboard_create(keyboards *owner, struct wlr_keyboard *wlr_keyboard);
static void keyboard_destroy(keyboard *keyboard);
static keyboard_prefs current_prefs(const keyboards *keyboards);
static void keyboard_apply_prefs(keyboard *keyboard, const keyboard_prefs *prefs);
static void lock_numlock(struct wlr_keyboard *keyboard);
static void handle_key(struct wl_listener *listener, void *data);
static void handle_modifiers(struct wl_listener *listener, void *data);
static void handle_destroy(struct wl_listener *listener, void *data);
static bool switch_vt(keyboard *keyboard, const struct wlr_keyboard_key_event *event);
static wm_dispatch dispatch_key(keyboard *keyboard, const struct wlr_keyboard_key_event *event);
static bool core_receives_keys(const seat *seat);
static uint32_t base_keysym(struct wlr_keyboard *keyboard, uint32_t keycode);
static bool take_consumed(keyboard *keyboard, uint32_t keycode);
static void remember_consumed(keyboard *keyboard, uint32_t keycode);
static void forward_key(keyboard *keyboard, const struct wlr_keyboard_key_event *event);

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
    keymap_cache_update(kbs->keymaps, &prefs);
    keyboard *kb;
    wl_list_for_each(kb, &kbs->list, link) {
        if (kb->follows_prefs) {
            keyboard_apply_prefs(kb, &prefs);
        }
    }
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

static void keyboard_apply_prefs(keyboard *kb, const keyboard_prefs *prefs)
{
    struct xkb_keymap *keymap = keymap_cache_keymap(kb->owner->keymaps);
    if (keymap != NULL && kb->wlr_keyboard->keymap != keymap) {
        wlr_keyboard_set_keymap(kb->wlr_keyboard, keymap);
    }
    int rate = prefs->repeat_rate > 0 ? prefs->repeat_rate : DEFAULT_REPEAT_RATE;
    int delay = prefs->repeat_delay_ms > 0 ? prefs->repeat_delay_ms : DEFAULT_REPEAT_DELAY_MS;
    wlr_keyboard_set_repeat_info(kb->wlr_keyboard, rate, delay);
    if (prefs->numlock) {
        lock_numlock(kb->wlr_keyboard);
    }
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
    if (switch_vt(kb, event)) {
        return;
    }
    if (dispatch_key(kb, event) == WM_FORWARD) {
        forward_key(kb, event);
    }
}

static void handle_modifiers(struct wl_listener *listener, void *data)
{
    (void) data;
    keyboard *kb = wl_container_of(listener, kb, modifiers);
    seat *s = kb->owner->seat;
    wlr_seat_set_keyboard(s->wlr_seat, kb->wlr_keyboard);
    wlr_seat_keyboard_notify_modifiers(s->wlr_seat, &kb->wlr_keyboard->modifiers);
    uint32_t mods = input_mapping_modifiers(wlr_keyboard_get_modifiers(kb->wlr_keyboard));
    wm_modifiers_changed(s->server->desktop, mods);
}

static void handle_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    keyboard *kb = wl_container_of(listener, kb, destroy);
    keyboard_destroy(kb);
}

static bool switch_vt(keyboard *kb, const struct wlr_keyboard_key_event *event)
{
    struct wlr_keyboard *wlr_keyboard = kb->wlr_keyboard;
    if (wlr_keyboard->xkb_state == NULL) {
        return false;
    }
    xkb_keycode_t code = event->keycode + XKB_KEYCODE_OFFSET;
    key_chord chord = {
        .translated_keysym = xkb_state_key_get_one_sym(wlr_keyboard->xkb_state, code),
        .base_keysym = base_keysym(wlr_keyboard, event->keycode),
        .modifiers = input_mapping_modifiers(wlr_keyboard_get_modifiers(wlr_keyboard)),
    };
    unsigned vt = input_mapping_vt(&chord);
    struct wlr_session *session = kb->owner->seat->server->session;
    if (vt == 0 || session == NULL) {
        return false;
    }
    if (event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        wlr_session_change_vt(session, vt);
    }
    return true;
}

static wm_dispatch dispatch_key(keyboard *kb, const struct wlr_keyboard_key_event *event)
{
    seat *s = kb->owner->seat;
    if (!core_receives_keys(s)) {
        return WM_FORWARD;
    }
    wm_key_event key = {
        .keysym = input_mapping_keysym(base_keysym(kb->wlr_keyboard, event->keycode)),
        .modifiers = input_mapping_modifiers(wlr_keyboard_get_modifiers(kb->wlr_keyboard)),
        .time_ms = event->time_msec,
    };
    if (event->state != WL_KEYBOARD_KEY_STATE_PRESSED) {
        wm_key_released(s->server->desktop, &key);
        return take_consumed(kb, event->keycode) ? WM_CONSUMED : WM_FORWARD;
    }
    wm_dispatch result = wm_key_pressed(s->server->desktop, &key);
    if (result == WM_CONSUMED) {
        remember_consumed(kb, event->keycode);
    }
    return result;
}

static bool core_receives_keys(const seat *s)
{
    if (seat_input_route(s) == INPUT_ROUTE_LOCKED) {
        return false;
    }
    return !shortcuts_inhibited(s->shortcuts, s->focus.current);
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

static bool take_consumed(keyboard *kb, uint32_t keycode)
{
    for (size_t i = 0; i < kb->consumed_count; i++) {
        if (kb->consumed[i] == keycode) {
            kb->consumed[i] = kb->consumed[--kb->consumed_count];
            return true;
        }
    }
    return false;
}

static void remember_consumed(keyboard *kb, uint32_t keycode)
{
    if (kb->consumed_count < CONSUMED_KEYS_MAX) {
        kb->consumed[kb->consumed_count++] = keycode;
    }
}

static void forward_key(keyboard *kb, const struct wlr_keyboard_key_event *event)
{
    struct wlr_seat *wlr_seat = kb->owner->seat->wlr_seat;
    wlr_seat_set_keyboard(wlr_seat, kb->wlr_keyboard);
    wlr_seat_keyboard_notify_key(wlr_seat, event->time_msec, event->keycode, event->state);
}
