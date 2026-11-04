#include <math.h>
#include <stdlib.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_touch.h>

#include "absolute_input.h"
#include "listener.h"
#include "pointer_target.h"
#include "seat_private.h"

enum {
    TOUCH_POINTS_MAX = 16,
};

typedef struct touch_point {
    touch_input *owner;
    bool used;
    int32_t id;
    struct wlr_scene_node *node;
    struct wl_listener node_destroy;
} touch_point;

struct touch_input {
    seat *seat;
    touch_point points[TOUCH_POINTS_MAX];
    struct wl_listener down;
    struct wl_listener up;
    struct wl_listener motion;
    struct wl_listener cancel;
    struct wl_listener frame;
};

static void handle_down(struct wl_listener *listener, void *data);
static void handle_up(struct wl_listener *listener, void *data);
static void handle_motion(struct wl_listener *listener, void *data);
static void handle_cancel(struct wl_listener *listener, void *data);
static void handle_frame(struct wl_listener *listener, void *data);
static void handle_node_destroy(struct wl_listener *listener, void *data);
static layout_point to_layout(const touch_input *touch, struct wlr_touch *device,
                              layout_point_fraction where);
static void tap_window(touch_input *touch, const pointer_hit *hit,
                       const struct wlr_touch_down_event *event);
static touch_point *point_find(touch_input *touch, int32_t id);
static touch_point *point_claim(touch_input *touch, int32_t id, struct wlr_scene_node *node);
static void point_release(touch_point *point);

touch_input *touch_input_create(seat *s)
{
    touch_input *touch = calloc(1, sizeof(*touch));
    if (touch == NULL) {
        return NULL;
    }
    touch->seat = s;
    for (int i = 0; i < TOUCH_POINTS_MAX; i++) {
        touch->points[i].owner = touch;
        wl_list_init(&touch->points[i].node_destroy.link);
    }
    listener_connect(&touch->down, &s->cursor->events.touch_down, handle_down);
    listener_connect(&touch->up, &s->cursor->events.touch_up, handle_up);
    listener_connect(&touch->motion, &s->cursor->events.touch_motion, handle_motion);
    listener_connect(&touch->cancel, &s->cursor->events.touch_cancel, handle_cancel);
    listener_connect(&touch->frame, &s->cursor->events.touch_frame, handle_frame);
    return touch;
}

void touch_input_destroy(touch_input *touch)
{
    for (int i = 0; i < TOUCH_POINTS_MAX; i++) {
        point_release(&touch->points[i]);
    }
    listener_disconnect(&touch->down);
    listener_disconnect(&touch->up);
    listener_disconnect(&touch->motion);
    listener_disconnect(&touch->cancel);
    listener_disconnect(&touch->frame);
    free(touch);
}

static void handle_down(struct wl_listener *listener, void *data)
{
    touch_input *touch = wl_container_of(listener, touch, down);
    const struct wlr_touch_down_event *event = data;
    seat *s = touch->seat;
    seat_notify_activity(s);
    layout_point_fraction where = {event->x, event->y};
    layout_point point = to_layout(touch, event->touch, where);
    pointer_hit hit = pointer_target_at(s->server, point);
    tap_window(touch, &hit, event);
    if (hit.surface == NULL || !wlr_surface_accepts_touch(hit.surface, s->wlr_seat)) {
        return;
    }
    if (point_claim(touch, event->touch_id, hit.node) != NULL) {
        wlr_seat_touch_notify_down(s->wlr_seat, hit.surface, event->time_msec, event->touch_id,
                                   hit.sx, hit.sy);
    }
}

static void handle_up(struct wl_listener *listener, void *data)
{
    touch_input *touch = wl_container_of(listener, touch, up);
    const struct wlr_touch_up_event *event = data;
    touch_point *point = point_find(touch, event->touch_id);
    if (point == NULL) {
        return;
    }
    wlr_seat_touch_notify_up(touch->seat->wlr_seat, event->time_msec, event->touch_id);
    point_release(point);
}

static void handle_motion(struct wl_listener *listener, void *data)
{
    touch_input *touch = wl_container_of(listener, touch, motion);
    const struct wlr_touch_motion_event *event = data;
    touch_point *point = point_find(touch, event->touch_id);
    if (point == NULL || point->node == NULL) {
        return;
    }
    layout_point_fraction where = {event->x, event->y};
    layout_point position = to_layout(touch, event->touch, where);
    int node_x = 0;
    int node_y = 0;
    wlr_scene_node_coords(point->node, &node_x, &node_y);
    wlr_seat_touch_notify_motion(touch->seat->wlr_seat, event->time_msec, event->touch_id,
                                 position.x - node_x, position.y - node_y);
}

static void handle_cancel(struct wl_listener *listener, void *data)
{
    touch_input *touch = wl_container_of(listener, touch, cancel);
    const struct wlr_touch_cancel_event *event = data;
    touch_point *point = point_find(touch, event->touch_id);
    if (point == NULL) {
        return;
    }
    struct wlr_touch_point *seat_point =
        wlr_seat_touch_get_point(touch->seat->wlr_seat, event->touch_id);
    if (seat_point != NULL) {
        wlr_seat_touch_notify_cancel(touch->seat->wlr_seat, seat_point->client);
    }
    point_release(point);
}

static void handle_frame(struct wl_listener *listener, void *data)
{
    (void) data;
    touch_input *touch = wl_container_of(listener, touch, frame);
    wlr_seat_touch_notify_frame(touch->seat->wlr_seat);
}

static void handle_node_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    touch_point *point = wl_container_of(listener, point, node_destroy);
    listener_disconnect(&point->node_destroy);
    point->node = NULL;
}

static layout_point to_layout(const touch_input *touch, struct wlr_touch *device,
                              layout_point_fraction where)
{
    layout_point point = {0};
    input_point mapped = absolute_input_position(touch->seat->absolute, &device->base,
                                                 (input_point){where.x, where.y});
    wlr_cursor_absolute_to_layout_coords(touch->seat->cursor, &device->base, mapped.x, mapped.y,
                                         &point.x, &point.y);
    return point;
}

static void tap_window(touch_input *touch, const pointer_hit *hit,
                       const struct wlr_touch_down_event *event)
{
    seat *s = touch->seat;
    wm_desktop *desktop = s->server->desktop;
    if (hit->target.window == NULL || seat_input_route(s) != INPUT_ROUTE_NORMAL) {
        return;
    }
    layout_point_fraction where = {event->x, event->y};
    layout_point point = to_layout(touch, event->touch, where);
    wm_button_event click = {
        .pointer =
            {
                .position = {(int) floor(point.x), (int) floor(point.y)},
                .time_ms = event->time_msec,
                .modifiers = seat_modifiers(s),
                .target = hit->target,
            },
        .button = WM_MOUSE_LEFT,
        .pressed = true,
    };
    wm_pointer_button(desktop, &click);
    click.pressed = false;
    wm_pointer_button(desktop, &click);
}

static touch_point *point_find(touch_input *touch, int32_t id)
{
    for (int i = 0; i < TOUCH_POINTS_MAX; i++) {
        if (touch->points[i].used && touch->points[i].id == id) {
            return &touch->points[i];
        }
    }
    return NULL;
}

static touch_point *point_claim(touch_input *touch, int32_t id, struct wlr_scene_node *node)
{
    for (int i = 0; i < TOUCH_POINTS_MAX; i++) {
        touch_point *point = &touch->points[i];
        if (!point->used) {
            point->used = true;
            point->id = id;
            point->node = node;
            listener_connect(&point->node_destroy, &node->events.destroy, handle_node_destroy);
            return point;
        }
    }
    return NULL;
}

static void point_release(touch_point *point)
{
    listener_disconnect(&point->node_destroy);
    point->used = false;
    point->node = NULL;
}
