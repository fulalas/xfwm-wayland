#include <math.h>
#include <stdlib.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_pointer_constraints_v1.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/util/region.h>

#include "util/listener.h"
#include "xwayland/xwayland.h"
#include "seat_private.h"

struct constraints {
    seat *seat;
    struct wlr_pointer_constraints_v1 *protocol;
    struct wlr_pointer_constraint_v1 *active;
    struct wl_listener new_constraint;
    struct wl_listener focus_change;
    struct wl_listener keyboard_focus_change;
    struct wl_listener active_destroy;
};

static void handle_new_constraint(struct wl_listener *listener, void *data);
static void handle_focus_change(struct wl_listener *listener, void *data);
static void handle_keyboard_focus_change(struct wl_listener *listener, void *data);
static void handle_active_destroy(struct wl_listener *listener, void *data);
static void refresh(constraints *constraints);
static struct wlr_pointer_constraint_v1 *focused_constraint(const constraints *constraints);
static void activate(constraints *constraints, struct wlr_pointer_constraint_v1 *constraint);
static void deactivate(constraints *constraints);
static void warp_to_hint(constraints *constraints);
static bool pointer_in_region(const constraints *constraints);

constraints *constraints_create(seat *s)
{
    constraints *c = calloc(1, sizeof(*c));
    if (c == NULL) {
        return NULL;
    }
    c->seat = s;
    c->protocol = s->server->globals.constraints;
    wl_list_init(&c->new_constraint.link);
    wl_list_init(&c->active_destroy.link);
    if (c->protocol != NULL) {
        listener_connect(&c->new_constraint, &c->protocol->events.new_constraint,
                         handle_new_constraint);
    }
    listener_connect(&c->focus_change, &s->wlr_seat->pointer_state.events.focus_change,
                     handle_focus_change);
    listener_connect(&c->keyboard_focus_change, &s->wlr_seat->keyboard_state.events.focus_change,
                     handle_keyboard_focus_change);
    return c;
}

void constraints_destroy(constraints *c)
{
    listener_disconnect(&c->new_constraint);
    listener_disconnect(&c->focus_change);
    listener_disconnect(&c->keyboard_focus_change);
    listener_disconnect(&c->active_destroy);
    free(c);
}

motion_delta constraints_filter_motion(const constraints *c, motion_delta delta)
{
    if (c->active == NULL || !pointer_in_region(c)) {
        return delta;
    }
    motion_delta still = {0, 0};
    if (c->active->type == WLR_POINTER_CONSTRAINT_V1_LOCKED) {
        return still;
    }
    const struct wlr_seat_pointer_state *pointer = &c->seat->wlr_seat->pointer_state;
    double scale = xwayland_surface_scale(c->seat->server, c->active->surface);
    double x = 0.0;
    double y = 0.0;
    if (!wlr_region_confine(&c->active->region, pointer->sx, pointer->sy,
                            pointer->sx + delta.dx * scale, pointer->sy + delta.dy * scale, &x,
                            &y)) {
        return still;
    }
    return (motion_delta){(x - pointer->sx) / scale, (y - pointer->sy) / scale};
}

static void handle_new_constraint(struct wl_listener *listener, void *data)
{
    (void) data;
    constraints *c = wl_container_of(listener, c, new_constraint);
    refresh(c);
}

static void handle_focus_change(struct wl_listener *listener, void *data)
{
    (void) data;
    constraints *c = wl_container_of(listener, c, focus_change);
    refresh(c);
}

static void handle_keyboard_focus_change(struct wl_listener *listener, void *data)
{
    (void) data;
    constraints *c = wl_container_of(listener, c, keyboard_focus_change);
    refresh(c);
}

static void handle_active_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    constraints *c = wl_container_of(listener, c, active_destroy);
    listener_disconnect(&c->active_destroy);
    warp_to_hint(c);
    c->active = NULL;
}

static void refresh(constraints *c)
{
    activate(c, focused_constraint(c));
}

static struct wlr_pointer_constraint_v1 *focused_constraint(const constraints *c)
{
    struct wlr_seat *wlr_seat = c->seat->wlr_seat;
    struct wlr_surface *surface = wlr_seat->pointer_state.focused_surface;
    if (surface == NULL || c->protocol == NULL ||
        wlr_surface_get_root_surface(surface) != wlr_seat->keyboard_state.focused_surface) {
        return NULL;
    }
    return wlr_pointer_constraints_v1_constraint_for_surface(c->protocol, surface, wlr_seat);
}

static void activate(constraints *c, struct wlr_pointer_constraint_v1 *constraint)
{
    if (c->active == constraint) {
        return;
    }
    deactivate(c);
    c->active = constraint;
    if (constraint == NULL) {
        return;
    }
    listener_connect(&c->active_destroy, &constraint->events.destroy, handle_active_destroy);
    wlr_pointer_constraint_v1_send_activated(constraint);
}

static void deactivate(constraints *c)
{
    if (c->active == NULL) {
        return;
    }
    listener_disconnect(&c->active_destroy);
    warp_to_hint(c);
    wlr_pointer_constraint_v1_send_deactivated(c->active);
    c->active = NULL;
}

static void warp_to_hint(constraints *c)
{
    struct wlr_pointer_constraint_v1 *active = c->active;
    if (active->type != WLR_POINTER_CONSTRAINT_V1_LOCKED || !active->current.cursor_hint.enabled) {
        return;
    }
    const struct wlr_seat_pointer_state *pointer = &c->seat->wlr_seat->pointer_state;
    if (pointer->focused_surface != active->surface) {
        return;
    }
    struct wlr_cursor *cursor = c->seat->cursor;
    double scale = xwayland_surface_scale(c->seat->server, active->surface);
    double origin_x = cursor->x - pointer->sx / scale;
    double origin_y = cursor->y - pointer->sy / scale;
    wlr_cursor_warp(cursor, NULL, origin_x + active->current.cursor_hint.x / scale,
                    origin_y + active->current.cursor_hint.y / scale);
}

static bool pointer_in_region(const constraints *c)
{
    const struct wlr_seat_pointer_state *pointer = &c->seat->wlr_seat->pointer_state;
    int x = (int) floor(pointer->sx);
    int y = (int) floor(pointer->sy);
    return pixman_region32_contains_point(&c->active->region, x, y, NULL);
}
