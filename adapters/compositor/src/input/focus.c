#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_seat.h>

#include "shell/layer_shell.h"
#include "server/lock.h"
#include "osd/osd.h"
#include "seat_private.h"

static struct wlr_surface *focus_target(const seat *seat);
static void keyboard_enter(seat *seat, struct wlr_surface *surface, bool past_grab);

void seat_focus_view(seat *s, view *v)
{
    if (s->focus.view != v) {
        if (s->focus.view != NULL) {
            view_unfocus(s->focus.view);
        }
        s->focus.view = v;
        if (v != NULL) {
            view_focus(v);
        }
    }
    s->focus.layer = NULL;
    s->focus.unmanaged = NULL;
    seat_focus_refresh(s);
}

void seat_focus_clicked_view(seat *s)
{
    if (s->focus.layer != NULL || s->focus.unmanaged != NULL) {
        s->focus.layer = NULL;
        s->focus.unmanaged = NULL;
        seat_focus_refresh(s);
    }
}

view *seat_focused_view(const seat *s)
{
    return s != NULL ? s->focus.view : NULL;
}

void seat_focus_layer(seat *s, struct wlr_surface *surface)
{
    s->focus.layer = surface;
    seat_focus_refresh(s);
}

void seat_focus_popup_grab(seat *s, struct wlr_surface *root)
{
    if (root == NULL || !wlr_seat_keyboard_has_grab(s->wlr_seat)) {
        return;
    }
    s->focus.popup = root;
    seat_focus_refresh(s);
}

void seat_focus_grab_ended(seat *s)
{
    s->focus.popup = NULL;
    seat_focus_refresh(s);
}

void seat_focus_reenter(seat *s)
{
    struct wlr_surface *surface = s->wlr_seat->keyboard_state.focused_surface;
    if (surface == NULL) {
        return;
    }
    wlr_seat_keyboard_clear_focus(s->wlr_seat);
    keyboard_enter(s, surface, true);
}

void seat_focus_unmanaged(seat *s, struct wlr_surface *surface)
{
    s->focus.unmanaged = surface;
    seat_focus_refresh(s);
}

void seat_surface_gone(seat *s, struct wlr_surface *surface)
{
    if (s->focus.layer == surface) {
        s->focus.layer = NULL;
    }
    if (s->focus.popup == surface) {
        s->focus.popup = NULL;
    }
    if (s->focus.unmanaged == surface) {
        s->focus.unmanaged = NULL;
    }
    if (s->focus.current == surface) {
        s->focus.current = NULL;
        wlr_seat_keyboard_notify_clear_focus(s->wlr_seat);
    }
    pointer_input_surface_gone(s->pointer, surface);
    seat_focus_refresh(s);
}

void seat_view_destroyed(seat *s, view *v)
{
    if (s == NULL) {
        return;
    }
    struct wlr_surface *surface = view_surface(v);
    if (s->focus.view == v) {
        s->focus.view = NULL;
    }
    if (surface != NULL) {
        seat_surface_gone(s, surface);
    }
}

void seat_focus_refresh(seat *s)
{
    struct wlr_surface *target = focus_target(s);
    if (target == s->focus.current && target == s->wlr_seat->keyboard_state.focused_surface) {
        return;
    }
    keyboard_enter(s, target, target != NULL && target == s->focus.popup);
    s->focus.current = target;
    shortcuts_focus_changed(s->shortcuts, target);
}

static struct wlr_surface *focus_target(const seat *s)
{
    compositor *server = s->server;
    if (seat_input_route(s) == INPUT_ROUTE_LOCKED) {
        return session_lock_focus(server->lock);
    }
    struct wlr_surface *exclusive =
        server->layer_shell != NULL ? layer_shell_exclusive_focus(server->layer_shell) : NULL;
    if (exclusive != NULL) {
        return exclusive;
    }
    if (server->osd != NULL && osd_is_modal(server->osd)) {
        return s->focus.current;
    }
    if (s->focus.popup != NULL) {
        return s->focus.popup;
    }
    if (s->focus.layer != NULL) {
        return s->focus.layer;
    }
    if (s->focus.unmanaged != NULL) {
        return s->focus.unmanaged;
    }
    return s->focus.view != NULL ? view_surface(s->focus.view) : NULL;
}

static void keyboard_enter(seat *s, struct wlr_surface *surface, bool past_grab)
{
    struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(s->wlr_seat);
    uint32_t keys[WLR_KEYBOARD_KEYS_CAP];
    size_t count = 0;
    const struct wlr_keyboard_modifiers *modifiers = NULL;
    if (surface != NULL && keyboard != NULL) {
        count = keyboards_forwarded_keys(s->keyboards, keyboard, keys);
        modifiers = &keyboard->modifiers;
    }
    if (past_grab) {
        wlr_seat_keyboard_enter(s->wlr_seat, surface, keys, count, modifiers);
    } else if (surface == NULL) {
        wlr_seat_keyboard_notify_clear_focus(s->wlr_seat);
    } else {
        wlr_seat_keyboard_notify_enter(s->wlr_seat, surface, keys, count, modifiers);
    }
}
