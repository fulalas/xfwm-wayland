#include <stdlib.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_relative_pointer_v1.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>

#include "click_focus.h"
#include "input_mapping.h"
#include "util/listener.h"
#include "pointer_target.h"
#include "seat_private.h"

enum {
    FORWARDED_BUTTONS_MAX = 16,
    HELD_BUTTONS_MAX = 16,
    AXIS_COUNT = 2,
    US_PER_MS = 1000,
};

typedef struct forwarded_button {
    uint32_t button;
    struct wl_client *client;
} forwarded_button;

typedef struct held_button {
    uint32_t button;
    const struct wlr_input_device *device;
} held_button;

struct pointer_input {
    seat *seat;
    struct wlr_scene_node *grab_node;
    struct wlr_surface *grab_surface;
    forwarded_button forwarded[FORWARDED_BUTTONS_MAX];
    size_t forwarded_count;
    held_button held[HELD_BUTTONS_MAX];
    size_t held_count;
    int32_t axis_remainder[AXIS_COUNT];
    struct wl_listener grab_node_destroy;
    struct wl_listener motion;
    struct wl_listener motion_absolute;
    struct wl_listener button;
    struct wl_listener axis;
    struct wl_listener frame;
};

typedef struct relative_motion {
    uint32_t time_ms;
    motion_delta accelerated;
    motion_delta unaccelerated;
} relative_motion;

static void handle_motion(struct wl_listener *listener, void *data);
static void handle_motion_absolute(struct wl_listener *listener, void *data);
static void handle_button(struct wl_listener *listener, void *data);
static void handle_axis(struct wl_listener *listener, void *data);
static void handle_frame(struct wl_listener *listener, void *data);
static void handle_grab_node_destroy(struct wl_listener *listener, void *data);
static void move_cursor(pointer_input *pointer, struct wlr_input_device *device,
                        const relative_motion *motion);
static bool core_receives_pointer(const pointer_input *pointer);
static void deliver_motion(pointer_input *pointer, const pointer_hit *hit, uint32_t time_ms);
static void deliver_grab_motion(pointer_input *pointer, uint32_t time_ms);
static void leave_surfaces(pointer_input *pointer);
static void press(pointer_input *pointer, const button_input *input);
static void press_grabbed(pointer_input *pointer, const pointer_hit *hit,
                          const button_input *input);
static bool forward_press(pointer_input *pointer, const button_input *input);
static void release(pointer_input *pointer, const button_input *input);
static wm_dispatch dispatch_button(pointer_input *pointer, const pointer_hit *hit,
                                   const button_input *input);
static void start_grab(pointer_input *pointer, const pointer_hit *hit);
static void close_client_menus(pointer_input *pointer);
static bool client_holds_pointer(const pointer_input *pointer);
static void end_grab(pointer_input *pointer);
static bool take_forwarded(pointer_input *pointer, uint32_t button, struct wl_client **client);
static struct wl_client *focused_client(const pointer_input *pointer);
static bool seat_holds_button(const struct wlr_seat *seat, uint32_t button);
static void note_held(pointer_input *pointer, const button_input *input);
static void release_held_at(pointer_input *pointer, size_t index, uint32_t time_ms);
static void handle_button_input(pointer_input *pointer, const button_input *input);
static bool is_held(const pointer_input *pointer, uint32_t button);
static wm_dispatch dispatch_axis(pointer_input *pointer,
                                 const struct wlr_pointer_axis_event *event);

pointer_input *pointer_input_create(seat *s)
{
    pointer_input *p = calloc(1, sizeof(*p));
    if (p == NULL) {
        return NULL;
    }
    p->seat = s;
    wl_list_init(&p->grab_node_destroy.link);
    listener_connect(&p->motion, &s->cursor->events.motion, handle_motion);
    listener_connect(&p->motion_absolute, &s->cursor->events.motion_absolute,
                     handle_motion_absolute);
    listener_connect(&p->button, &s->cursor->events.button, handle_button);
    listener_connect(&p->axis, &s->cursor->events.axis, handle_axis);
    listener_connect(&p->frame, &s->cursor->events.frame, handle_frame);
    return p;
}

void pointer_input_destroy(pointer_input *p)
{
    end_grab(p);
    listener_disconnect(&p->motion);
    listener_disconnect(&p->motion_absolute);
    listener_disconnect(&p->button);
    listener_disconnect(&p->axis);
    listener_disconnect(&p->frame);
    free(p);
}

void pointer_input_motion(pointer_input *p, uint32_t time_ms)
{
    seat *s = p->seat;
    selection_move_icons(s->selection);
    if (seat_input_route(s) == INPUT_ROUTE_LOCKED) {
        pointer_input_release_grab(p);
    }
    if (p->grab_surface != NULL && s->wlr_seat->drag == NULL) {
        deliver_grab_motion(p, time_ms);
        return;
    }
    pointer_hit hit = seat_hit_at_cursor(p->seat);
    wm_dispatch result = WM_FORWARD;
    if (core_receives_pointer(p)) {
        wm_pointer_event event = seat_core_event(p->seat, &hit, time_ms);
        result = wm_pointer_motion(s->server->desktop, &event);
    }
    if (result == WM_FORWARD && hit.surface != NULL) {
        deliver_motion(p, &hit, time_ms);
    } else {
        leave_surfaces(p);
    }
}

void pointer_input_button(pointer_input *p, const button_input *input)
{
    seat_notify_activity(p->seat);
    handle_button_input(p, input);
}

void pointer_input_surface_gone(pointer_input *p, struct wlr_surface *surface)
{
    if (p->grab_surface == surface) {
        end_grab(p);
    }
}

void pointer_input_release_device(pointer_input *p, const struct wlr_input_device *device,
                                  uint32_t time_ms)
{
    for (size_t i = p->held_count; i-- > 0;) {
        if (p->held[i].device == device) {
            release_held_at(p, i, time_ms);
        }
    }
}

void pointer_input_release_all(pointer_input *p, uint32_t time_ms)
{
    while (p->held_count > 0) {
        release_held_at(p, p->held_count - 1, time_ms);
    }
}

void pointer_input_release_grab(pointer_input *p)
{
    if (p->grab_surface == NULL) {
        return;
    }
    end_grab(p);
    leave_surfaces(p);
}

static void handle_motion(struct wl_listener *listener, void *data)
{
    pointer_input *p = wl_container_of(listener, p, motion);
    const struct wlr_pointer_motion_event *event = data;
    relative_motion motion = {
        .time_ms = event->time_msec,
        .accelerated = {event->delta_x, event->delta_y},
        .unaccelerated = {event->unaccel_dx, event->unaccel_dy},
    };
    move_cursor(p, &event->pointer->base, &motion);
}

static void handle_motion_absolute(struct wl_listener *listener, void *data)
{
    pointer_input *p = wl_container_of(listener, p, motion_absolute);
    const struct wlr_pointer_motion_absolute_event *event = data;
    struct wlr_cursor *cursor = p->seat->cursor;
    double lx = 0;
    double ly = 0;
    wlr_cursor_absolute_to_layout_coords(cursor, &event->pointer->base, event->x, event->y, &lx,
                                         &ly);
    motion_delta delta = {lx - cursor->x, ly - cursor->y};
    relative_motion motion = {event->time_msec, delta, delta};
    move_cursor(p, &event->pointer->base, &motion);
}

static void handle_button(struct wl_listener *listener, void *data)
{
    pointer_input *p = wl_container_of(listener, p, button);
    const struct wlr_pointer_button_event *event = data;
    button_input input = {
        .button = event->button,
        .pressed = event->state == WL_POINTER_BUTTON_STATE_PRESSED,
        .time_ms = event->time_msec,
        .device = &event->pointer->base,
    };
    pointer_input_button(p, &input);
}

static void handle_axis(struct wl_listener *listener, void *data)
{
    pointer_input *p = wl_container_of(listener, p, axis);
    const struct wlr_pointer_axis_event *event = data;
    seat_notify_activity(p->seat);
    if (dispatch_axis(p, event) == WM_FORWARD) {
        wlr_seat_pointer_notify_axis(p->seat->wlr_seat, event->time_msec, event->orientation,
                                     event->delta, event->delta_discrete, event->source,
                                     event->relative_direction);
    }
}

static void handle_frame(struct wl_listener *listener, void *data)
{
    (void) data;
    pointer_input *p = wl_container_of(listener, p, frame);
    wlr_seat_pointer_notify_frame(p->seat->wlr_seat);
}

static void handle_grab_node_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    pointer_input *p = wl_container_of(listener, p, grab_node_destroy);
    end_grab(p);
}

static void move_cursor(pointer_input *p, struct wlr_input_device *device,
                        const relative_motion *motion)
{
    seat *s = p->seat;
    seat_notify_activity(s);
    struct wlr_relative_pointer_manager_v1 *relative = s->server->globals.relative_pointer;
    if (relative != NULL) {
        wlr_relative_pointer_manager_v1_send_relative_motion(
            relative, s->wlr_seat, (uint64_t) motion->time_ms * US_PER_MS,
            motion->accelerated.dx, motion->accelerated.dy, motion->unaccelerated.dx,
            motion->unaccelerated.dy);
    }
    motion_delta allowed = constraints_filter_motion(s->constraints, motion->accelerated);
    wlr_cursor_move(s->cursor, device, allowed.dx, allowed.dy);
    pointer_input_motion(p, motion->time_ms);
}

static bool core_receives_pointer(const pointer_input *p)
{
    const seat *s = p->seat;
    return seat_input_route(s) == INPUT_ROUTE_NORMAL;
}

static void deliver_motion(pointer_input *p, const pointer_hit *hit, uint32_t time_ms)
{
    struct wlr_seat *wlr_seat = p->seat->wlr_seat;
    wlr_seat_pointer_notify_enter(wlr_seat, hit->surface, hit->sx, hit->sy);
    wlr_seat_pointer_notify_motion(wlr_seat, time_ms, hit->sx, hit->sy);
    cursor_image_use_client(p->seat->image);
}

static void deliver_grab_motion(pointer_input *p, uint32_t time_ms)
{
    int node_x = 0;
    int node_y = 0;
    wlr_scene_node_coords(p->grab_node, &node_x, &node_y);
    struct wlr_cursor *cursor = p->seat->cursor;
    wlr_seat_pointer_notify_motion(p->seat->wlr_seat, time_ms, cursor->x - node_x,
                                   cursor->y - node_y);
}

static void leave_surfaces(pointer_input *p)
{
    wlr_seat_pointer_notify_clear_focus(p->seat->wlr_seat);
    cursor_image_use_core(p->seat->image);
}

static void press(pointer_input *p, const button_input *input)
{
    pointer_hit hit = seat_hit_at_cursor(p->seat);
    if (p->grab_surface != NULL) {
        press_grabbed(p, &hit, input);
        return;
    }
    wm_dispatch result = dispatch_button(p, &hit, input);
    click_focus(p->seat, &hit);
    if (result != WM_FORWARD || hit.surface == NULL) {
        close_client_menus(p);
        return;
    }
    struct wlr_seat *wlr_seat = p->seat->wlr_seat;
    if (wlr_seat->pointer_state.focused_surface != hit.surface) {
        deliver_motion(p, &hit, input->time_ms);
    }
    if (!forward_press(p, input)) {
        return;
    }
    start_grab(p, &hit);
}

static void press_grabbed(pointer_input *p, const pointer_hit *hit, const button_input *input)
{
    if (dispatch_button(p, hit, input) == WM_FORWARD) {
        forward_press(p, input);
    }
}

static bool forward_press(pointer_input *p, const button_input *input)
{
    if (p->forwarded_count == FORWARDED_BUTTONS_MAX) {
        return false;
    }
    p->forwarded[p->forwarded_count++] =
        (forwarded_button){input->button, focused_client(p)};
    wlr_seat_pointer_notify_button(p->seat->wlr_seat, input->time_ms, input->button,
                                   WL_POINTER_BUTTON_STATE_PRESSED);
    return true;
}

static void release(pointer_input *p, const button_input *input)
{
    pointer_hit hit = seat_hit_at_cursor(p->seat);
    dispatch_button(p, &hit, input);
    struct wl_client *client = NULL;
    if (!take_forwarded(p, input->button, &client)) {
        return;
    }
    struct wlr_seat *wlr_seat = p->seat->wlr_seat;
    uint32_t serial = wlr_seat_pointer_notify_button(wlr_seat, input->time_ms, input->button,
                                                     WL_POINTER_BUTTON_STATE_RELEASED);
    if (serial == 0 && client != NULL && focused_client(p) == client &&
        !seat_holds_button(wlr_seat, input->button)) {
        wlr_seat_pointer_send_button(wlr_seat, input->time_ms, input->button,
                                     WL_POINTER_BUTTON_STATE_RELEASED);
    }
    if (p->forwarded_count == 0) {
        end_grab(p);
        pointer_input_motion(p, input->time_ms);
    }
}

static wm_dispatch dispatch_button(pointer_input *p, const pointer_hit *hit,
                                   const button_input *input)
{
    uint32_t button = input_mapping_button(input->button);
    if (button == 0 || (input->pressed && !core_receives_pointer(p))) {
        return WM_FORWARD;
    }
    wm_button_event event = {
        .pointer = seat_core_event(p->seat, hit, input->time_ms),
        .button = button,
        .pressed = input->pressed,
    };
    if (event.pointer.target.kind == WM_TARGET_DESKTOP && client_holds_pointer(p)) {
        event.pointer.target.kind = WM_TARGET_OTHER;
    }
    return wm_pointer_button(p->seat->server->desktop, &event);
}

static void close_client_menus(pointer_input *p)
{
    struct wlr_seat *wlr_seat = p->seat->wlr_seat;
    if (wlr_seat->drag == NULL && wlr_seat_pointer_has_grab(wlr_seat)) {
        wlr_seat_pointer_end_grab(wlr_seat);
    }
}

static bool client_holds_pointer(const pointer_input *p)
{
    return p->grab_surface != NULL || wlr_seat_pointer_has_grab(p->seat->wlr_seat);
}

static void start_grab(pointer_input *p, const pointer_hit *hit)
{
    if (p->grab_surface != NULL || hit->node == NULL) {
        return;
    }
    p->grab_surface = hit->surface;
    p->grab_node = hit->node;
    listener_connect(&p->grab_node_destroy, &hit->node->events.destroy, handle_grab_node_destroy);
}

static void end_grab(pointer_input *p)
{
    listener_disconnect(&p->grab_node_destroy);
    p->grab_surface = NULL;
    p->grab_node = NULL;
}

static bool take_forwarded(pointer_input *p, uint32_t button, struct wl_client **client)
{
    for (size_t i = 0; i < p->forwarded_count; i++) {
        if (p->forwarded[i].button == button) {
            *client = p->forwarded[i].client;
            p->forwarded[i] = p->forwarded[--p->forwarded_count];
            return true;
        }
    }
    return false;
}

static bool seat_holds_button(const struct wlr_seat *seat, uint32_t button)
{
    const struct wlr_seat_pointer_state *state = &seat->pointer_state;
    for (size_t i = 0; i < state->button_count; i++) {
        if (state->buttons[i].button == button) {
            return true;
        }
    }
    return false;
}

static struct wl_client *focused_client(const pointer_input *p)
{
    const struct wlr_seat_client *client = p->seat->wlr_seat->pointer_state.focused_client;
    return client != NULL ? client->client : NULL;
}

static wm_dispatch dispatch_axis(pointer_input *p, const struct wlr_pointer_axis_event *event)
{
    if (!core_receives_pointer(p)) {
        return WM_FORWARD;
    }
    int index = event->orientation == WL_POINTER_AXIS_VERTICAL_SCROLL ? 0 : 1;
    int32_t value120 = input_mapping_axis_value120(event->delta_discrete, event->delta);
    axis_steps steps = input_mapping_axis(p->axis_remainder[index], value120);
    p->axis_remainder[index] = steps.remainder;
    if (steps.steps == 0) {
        return WM_FORWARD;
    }
    pointer_hit hit = seat_hit_at_cursor(p->seat);
    wm_axis_event axis = {.pointer = seat_core_event(p->seat, &hit, event->time_msec)};
    if (index == 0) {
        axis.vertical_steps = steps.steps;
    } else {
        axis.horizontal_steps = steps.steps;
    }
    return wm_pointer_axis(p->seat->server->desktop, &axis);
}

static void note_held(pointer_input *p, const button_input *input)
{
    for (size_t i = 0; i < p->held_count; i++) {
        if (p->held[i].button == input->button) {
            if (!input->pressed) {
                p->held[i] = p->held[--p->held_count];
            }
            return;
        }
    }
    if (input->pressed && p->held_count < HELD_BUTTONS_MAX) {
        p->held[p->held_count++] = (held_button){input->button, input->device};
    }
}

static void release_held_at(pointer_input *p, size_t index, uint32_t time_ms)
{
    button_input input = {
        .button = p->held[index].button,
        .pressed = false,
        .time_ms = time_ms,
        .device = p->held[index].device,
    };
    handle_button_input(p, &input);
}

static void handle_button_input(pointer_input *p, const button_input *input)
{
    if (!input->pressed && !is_held(p, input->button)) {
        return;
    }
    note_held(p, input);
    if (input->pressed) {
        press(p, input);
    } else {
        release(p, input);
    }
}

static bool is_held(const pointer_input *p, uint32_t button)
{
    for (size_t i = 0; i < p->held_count; i++) {
        if (p->held[i].button == button) {
            return true;
        }
    }
    return false;
}
