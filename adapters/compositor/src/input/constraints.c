#include <math.h>
#include <stdlib.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_pointer_constraints_v1.h>
#include <wlr/types/wlr_seat.h>

#include "util/listener.h"
#include "seat_private.h"

struct constraints {
    seat *seat;
    struct wlr_pointer_constraints_v1 *protocol;
    struct wlr_pointer_constraint_v1 *active;
    struct wl_listener new_constraint;
    struct wl_listener focus_change;
    struct wl_listener active_destroy;
};

static void handle_new_constraint(struct wl_listener *listener, void *data);
static void handle_focus_change(struct wl_listener *listener, void *data);
static void handle_active_destroy(struct wl_listener *listener, void *data);
static void activate(constraints *constraints, struct wlr_pointer_constraint_v1 *constraint);
static void deactivate(constraints *constraints);
static void warp_to_hint(constraints *constraints);
static bool region_allows(const constraints *constraints, double dx, double dy);

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
    return c;
}

void constraints_destroy(constraints *c)
{
    listener_disconnect(&c->new_constraint);
    listener_disconnect(&c->focus_change);
    listener_disconnect(&c->active_destroy);
    free(c);
}

motion_delta constraints_filter_motion(const constraints *c, motion_delta delta)
{
    if (c->active == NULL) {
        return delta;
    }
    motion_delta still = {0, 0};
    if (c->active->type == WLR_POINTER_CONSTRAINT_V1_LOCKED) {
        return still;
    }
    if (region_allows(c, delta.dx, delta.dy)) {
        return delta;
    }
    if (region_allows(c, delta.dx, 0)) {
        return (motion_delta){delta.dx, 0};
    }
    if (region_allows(c, 0, delta.dy)) {
        return (motion_delta){0, delta.dy};
    }
    return still;
}

static void handle_new_constraint(struct wl_listener *listener, void *data)
{
    constraints *c = wl_container_of(listener, c, new_constraint);
    struct wlr_pointer_constraint_v1 *constraint = data;
    if (constraint->surface == c->seat->wlr_seat->pointer_state.focused_surface) {
        activate(c, constraint);
    }
}

static void handle_focus_change(struct wl_listener *listener, void *data)
{
    constraints *c = wl_container_of(listener, c, focus_change);
    const struct wlr_seat_pointer_focus_change_event *event = data;
    struct wlr_pointer_constraint_v1 *constraint = NULL;
    if (event->new_surface != NULL && c->protocol != NULL) {
        constraint = wlr_pointer_constraints_v1_constraint_for_surface(
            c->protocol, event->new_surface, c->seat->wlr_seat);
    }
    activate(c, constraint);
}

static void handle_active_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    constraints *c = wl_container_of(listener, c, active_destroy);
    listener_disconnect(&c->active_destroy);
    warp_to_hint(c);
    c->active = NULL;
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
    double origin_x = cursor->x - pointer->sx;
    double origin_y = cursor->y - pointer->sy;
    wlr_cursor_warp(cursor, NULL, origin_x + active->current.cursor_hint.x,
                    origin_y + active->current.cursor_hint.y);
}

static bool region_allows(const constraints *c, double dx, double dy)
{
    const struct wlr_seat_pointer_state *pointer = &c->seat->wlr_seat->pointer_state;
    int x = (int) floor(pointer->sx + dx);
    int y = (int) floor(pointer->sy + dy);
    return pixman_region32_contains_point(&c->active->region, x, y, NULL);
}
