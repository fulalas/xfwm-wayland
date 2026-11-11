#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_tablet_v2.h>

#include "click_focus.h"
#include "input_mapping.h"
#include "util/listener.h"
#include "xwayland/xwayland.h"
#include "pointer_target.h"
#include "tablet_private.h"

static bool holds_grab(const tablet_tool *tool);
static void move_grabbed(tablet_tool *tool);
static bool accepts_tablet(const tablet_tool *tool, const pointer_hit *hit,
                           const tool_event *event);
static void route_to_client(tablet_tool *tool, const pointer_hit *hit, const tool_event *event);
static void route_to_pointer(tablet_tool *tool, const tool_event *event);
static void report_motion_to_core(tablet_tool *tool, const pointer_hit *hit, uint32_t time_ms);
static void handle_grab_node_destroy(struct wl_listener *listener, void *data);

void tablet_route_update(tablet_tool *tool, const tool_event *event)
{
    if (holds_grab(tool)) {
        move_grabbed(tool);
        return;
    }
    if (tool->route == TOOL_ROUTE_EMULATED && tool->tip_down) {
        route_to_pointer(tool, event);
        return;
    }
    pointer_hit hit = seat_hit_at_cursor(tool->owner->seat);
    if (accepts_tablet(tool, &hit, event)) {
        route_to_client(tool, &hit, event);
    } else {
        route_to_pointer(tool, event);
    }
}

void tablet_route_drop_grab(tablet_tool *tool)
{
    listener_disconnect(&tool->grab_node_destroy);
    tool->grab_node = NULL;
}

void tablet_route_hold_target(tablet_tool *tool)
{
    pointer_hit hit = seat_hit_at_cursor(tool->owner->seat);
    if (hit.node == NULL) {
        return;
    }
    click_focus(tool->owner->seat, &hit);
    tablet_route_drop_grab(tool);
    tool->grab_node = hit.node;
    listener_connect(&tool->grab_node_destroy, &hit.node->events.destroy,
                     handle_grab_node_destroy);
}

wm_dispatch tablet_route_tip_to_core(tablet_tool *tool, const button_input *input)
{
    seat *s = tool->owner->seat;
    if (input->pressed && seat_input_route(s) != INPUT_ROUTE_NORMAL) {
        return WM_FORWARD;
    }
    pointer_hit hit = seat_hit_at_cursor(tool->owner->seat);
    wm_button_event event = {
        .pointer = seat_core_event(tool->owner->seat, &hit, input->time_ms),
        .button = input_mapping_button(input->button),
        .pressed = input->pressed,
    };
    return wm_pointer_button(s->server->desktop, &event);
}

static bool holds_grab(const tablet_tool *tool)
{
    return tool->route == TOOL_ROUTE_CLIENT && tool->grab_node != NULL &&
           (tool->v2->is_down || tool->v2->num_buttons > 0);
}

static void move_grabbed(tablet_tool *tool)
{
    struct wlr_cursor *cursor = tool->owner->seat->cursor;
    double sx = 0.0;
    double sy = 0.0;
    xwayland_node_local(tool->owner->seat->server, tool->grab_node, cursor->x, cursor->y, &sx,
                        &sy);
    wlr_tablet_v2_tablet_tool_notify_motion(tool->v2, sx, sy);
}

static bool accepts_tablet(const tablet_tool *tool, const pointer_hit *hit,
                           const tool_event *event)
{
    if (hit->surface == NULL || event->tablet->v2 == NULL) {
        return false;
    }
    if (seat_input_route(tool->owner->seat) == INPUT_ROUTE_DRAG) {
        return false;
    }
    return wlr_surface_accepts_tablet_v2(hit->surface, event->tablet->v2);
}

static void route_to_client(tablet_tool *tool, const pointer_hit *hit, const tool_event *event)
{
    tool->route = TOOL_ROUTE_CLIENT;
    wlr_tablet_v2_tablet_tool_notify_proximity_in(tool->v2, event->tablet->v2, hit->surface);
    wlr_tablet_v2_tablet_tool_notify_motion(tool->v2, hit->sx, hit->sy);
    cursor_image_use_client(tool->owner->seat->image);
    report_motion_to_core(tool, hit, event->time_ms);
}

static void route_to_pointer(tablet_tool *tool, const tool_event *event)
{
    if (tool->route == TOOL_ROUTE_CLIENT) {
        wlr_tablet_v2_tablet_tool_notify_proximity_out(tool->v2);
    }
    tool->route = TOOL_ROUTE_EMULATED;
    pointer_input_motion(tool->owner->seat->pointer, event->time_ms);
}

static void report_motion_to_core(tablet_tool *tool, const pointer_hit *hit, uint32_t time_ms)
{
    seat *s = tool->owner->seat;
    if (seat_input_route(s) == INPUT_ROUTE_NORMAL) {
        wm_pointer_event event = seat_core_event(tool->owner->seat, hit, time_ms);
        wm_pointer_motion(s->server->desktop, &event);
    }
}

static void handle_grab_node_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    tablet_tool *tool = wl_container_of(listener, tool, grab_node_destroy);
    tablet_route_drop_grab(tool);
}
