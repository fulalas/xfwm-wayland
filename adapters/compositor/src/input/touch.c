#include <math.h>
#include <stdlib.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_touch.h>

#include "absolute_input.h"
#include "click_focus.h"
#include "util/listener.h"
#include "xwayland/xwayland.h"
#include "pointer_target.h"
#include "seat_private.h"

enum {
    TOUCH_POINTS_MAX = 16,
};

typedef struct touch_point {
    bool used;
    int32_t id;
    layout_point position;
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
static layout_point to_layout(const touch_input *touch, struct wlr_touch *device, double x,
                              double y);
static void tap(touch_input *touch, const pointer_hit *hit, layout_point point,
                uint32_t time_ms);
static touch_point *point_find(touch_input *touch, int32_t id);
static touch_point *point_claim(touch_input *touch, int32_t id, struct wlr_scene_node *node,
                                layout_point position);
static void point_release(touch_point *point);

touch_input *touch_input_create(seat *s)
{
    touch_input *touch = calloc(1, sizeof(*touch));
    if (touch == NULL) {
        return NULL;
    }
    touch->seat = s;
    for (int i = 0; i < TOUCH_POINTS_MAX; i++) {
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

void touch_input_cancel_all(touch_input *touch)
{
    struct wlr_seat *wlr_seat = touch->seat->wlr_seat;
    int points = wlr_seat_touch_num_points(wlr_seat);
    for (int i = 0; i < points && !wl_list_empty(&wlr_seat->touch_state.touch_points); i++) {
        struct wlr_touch_point *point =
            wl_container_of(wlr_seat->touch_state.touch_points.next, point, link);
        wlr_seat_touch_notify_cancel(wlr_seat, point->client);
    }
    for (int i = 0; i < TOUCH_POINTS_MAX; i++) {
        point_release(&touch->points[i]);
    }
}

bool touch_input_position(touch_input *touch, int32_t id, layout_point *position)
{
    const touch_point *point = point_find(touch, id);
    if (point == NULL) {
        return false;
    }
    *position = point->position;
    return true;
}

static void handle_down(struct wl_listener *listener, void *data)
{
    touch_input *touch = wl_container_of(listener, touch, down);
    const struct wlr_touch_down_event *event = data;
    seat *s = touch->seat;
    seat_notify_activity(s);
    layout_point point = to_layout(touch, event->touch, event->x, event->y);
    pointer_hit hit = pointer_target_at(s->server, point);
    tap(touch, &hit, point, event->time_msec);
    if (hit.surface == NULL || !wlr_surface_accepts_touch(hit.surface, s->wlr_seat)) {
        return;
    }
    if (point_claim(touch, event->touch_id, hit.node, point) != NULL) {
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
    if (point == NULL) {
        return;
    }
    point->position = to_layout(touch, event->touch, event->x, event->y);
    selection_drag_moved(touch->seat->selection);
    if (point->node == NULL) {
        return;
    }
    double sx = 0.0;
    double sy = 0.0;
    xwayland_node_local(touch->seat->server, point->node, point->position.x, point->position.y,
                        &sx, &sy);
    wlr_seat_touch_notify_motion(touch->seat->wlr_seat, event->time_msec, event->touch_id, sx,
                                 sy);
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

static layout_point to_layout(const touch_input *touch, struct wlr_touch *device, double x,
                              double y)
{
    layout_point point = {0};
    input_point mapped =
        absolute_input_position(touch->seat->absolute, &device->base, (input_point){x, y});
    wlr_cursor_absolute_to_layout_coords(touch->seat->cursor, &device->base, mapped.x, mapped.y,
                                         &point.x, &point.y);
    return point;
}

static void tap(touch_input *touch, const pointer_hit *hit, layout_point point,
                uint32_t time_ms)
{
    seat *s = touch->seat;
    if (seat_input_route(s) != INPUT_ROUTE_NORMAL) {
        return;
    }
    wm_desktop *desktop = s->server->desktop;
    wm_button_event click = {
        .pointer =
            {
                .position = {(int) floor(point.x), (int) floor(point.y)},
                .time_ms = time_ms,
                .modifiers = seat_modifiers(s),
                .target = hit->target,
            },
        .button = WM_MOUSE_LEFT,
        .pressed = true,
    };
    wm_pointer_button(desktop, &click);
    click_focus(s, hit);
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

static touch_point *point_claim(touch_input *touch, int32_t id, struct wlr_scene_node *node,
                                layout_point position)
{
    for (int i = 0; i < TOUCH_POINTS_MAX; i++) {
        touch_point *point = &touch->points[i];
        if (!point->used) {
            point->used = true;
            point->id = id;
            point->node = node;
            point->position = position;
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
