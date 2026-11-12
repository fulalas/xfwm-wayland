#include <linux/input-event-codes.h>
#include <stdlib.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_tablet_tool.h>
#include <wlr/types/wlr_tablet_v2.h>
#include <wlr/util/log.h>

#include "absolute_input.h"
#include "util/clock.h"
#include "util/listener.h"
#include "tablet_private.h"

static tablet_tool *create_tool(tablet_input *tablets, struct wlr_tablet_tool *tool);
static void handle_tool_destroy(struct wl_listener *listener, void *data);
static void handle_set_cursor(struct wl_listener *listener, void *data);
static void remember_position(tablet_tool *tool, const struct wlr_tablet_tool_axis_event *axes);
static void warp(tablet_tool *tool, const tool_event *event);
static void release_grab_when_idle(tablet_tool *tool);
static void release_tip(tablet_tool *tool, uint32_t time_ms);
static uint32_t emulated_button(uint32_t stylus_button);

tablet_tool *tablet_tool_get(tablet_input *tablets, struct wlr_tablet_tool *tool)
{
    if (tool->data == NULL) {
        tool->data = create_tool(tablets, tool);
    }
    return tool->data;
}

void tablet_tool_free(tablet_tool *tool)
{
    release_tip(tool, (uint32_t) clock_now_ms());
    tablet_route_drop_grab(tool);
    listener_disconnect(&tool->set_cursor);
    listener_disconnect(&tool->destroy);
    wl_list_remove(&tool->link);
    tool->tool->data = NULL;
    free(tool);
}

void tablet_tool_enter(tablet_tool *tool, const tool_event *event)
{
    warp(tool, event);
    tablet_route_update(tool, event);
}

void tablet_tool_leave(tablet_tool *tool)
{
    release_tip(tool, (uint32_t) clock_now_ms());
    if (tool->route == TOOL_ROUTE_CLIENT) {
        wlr_tablet_v2_tablet_tool_notify_proximity_out(tool->v2);
    }
    tablet_route_drop_grab(tool);
    tool->route = TOOL_ROUTE_NONE;
}

void tablet_tool_axis(tablet_tool *tool, const tool_event *event,
                      const struct wlr_tablet_tool_axis_event *axes)
{
    if ((axes->updated_axes & (WLR_TABLET_TOOL_AXIS_X | WLR_TABLET_TOOL_AXIS_Y)) != 0) {
        remember_position(tool, axes);
        warp(tool, event);
        tablet_route_update(tool, event);
    }
    tablet_axes_send(tool, event, axes);
}

void tablet_tool_press_tip(tablet_tool *tool, const tool_event *event)
{
    button_input input = {
        .button = BTN_LEFT,
        .pressed = true,
        .time_ms = event->time_ms,
        .device = &event->tablet->tablet->base,
    };
    tool->tip_down = true;
    tool->tip_device = input.device;
    if (tool->route != TOOL_ROUTE_CLIENT) {
        pointer_input_button(tool->owner->seat->pointer, &input);
        return;
    }
    seat_notify_activity(tool->owner->seat);
    if (tablet_route_tip_to_core(tool, &input) != WM_FORWARD) {
        wlr_tablet_v2_tablet_tool_notify_proximity_out(tool->v2);
        tool->route = TOOL_ROUTE_EMULATED;
        tool->tip_in_core = true;
        return;
    }
    tablet_route_hold_target(tool);
    wlr_tablet_v2_tablet_tool_notify_down(tool->v2);
    wlr_tablet_tool_v2_start_implicit_grab(tool->v2);
}

void tablet_tool_release_tip(tablet_tool *tool, const tool_event *event)
{
    bool to_client = tool->route == TOOL_ROUTE_CLIENT;
    release_tip(tool, event->time_ms);
    if (to_client) {
        tablet_route_update(tool, event);
    }
}

void tablet_input_release_tips(tablet_input *tablets, uint32_t time_ms)
{
    tablet_tool *tool;
    wl_list_for_each(tool, &tablets->tools, link) {
        release_tip(tool, time_ms);
    }
}

bool tablet_input_holds_serial(const tablet_input *tablets, uint32_t serial)
{
    const tablet_tool *tool;
    wl_list_for_each(tool, &tablets->tools, link) {
        if (tool->route == TOOL_ROUTE_CLIENT && tool->v2->is_down &&
            tool->v2->down_serial == serial) {
            return true;
        }
    }
    return false;
}

void tablet_tool_button(tablet_tool *tool, const button_input *input)
{
    if (tool->route != TOOL_ROUTE_CLIENT) {
        button_input emulated = *input;
        emulated.button = emulated_button(input->button);
        pointer_input_button(tool->owner->seat->pointer, &emulated);
        return;
    }
    if (input->pressed && tool->grab_node == NULL) {
        tablet_route_hold_target(tool);
    }
    enum zwp_tablet_pad_v2_button_state state = input->pressed
                                                    ? ZWP_TABLET_PAD_V2_BUTTON_STATE_PRESSED
                                                    : ZWP_TABLET_PAD_V2_BUTTON_STATE_RELEASED;
    wlr_tablet_v2_tablet_tool_notify_button(tool->v2, input->button, state);
    release_grab_when_idle(tool);
}

static tablet_tool *create_tool(tablet_input *tablets, struct wlr_tablet_tool *tool)
{
    tablet_tool *t = calloc(1, sizeof(*t));
    if (t == NULL) {
        return NULL;
    }
    t->owner = tablets;
    t->tool = tool;
    wl_list_init(&t->grab_node_destroy.link);
    wl_list_init(&t->set_cursor.link);
    listener_connect(&t->destroy, &tool->events.destroy, handle_tool_destroy);
    t->v2 = wlr_tablet_tool_create(tablets->manager, tablets->seat->wlr_seat, tool);
    if (t->v2 == NULL) {
        wlr_log(WLR_ERROR, "cannot announce a tablet tool to clients");
        listener_disconnect(&t->destroy);
        free(t);
        return NULL;
    }
    listener_connect(&t->set_cursor, &t->v2->events.set_cursor, handle_set_cursor);
    wl_list_insert(&tablets->tools, &t->link);
    return t;
}

static void handle_tool_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    tablet_tool *tool = wl_container_of(listener, tool, destroy);
    tablet_tool_free(tool);
}

static void handle_set_cursor(struct wl_listener *listener, void *data)
{
    tablet_tool *tool = wl_container_of(listener, tool, set_cursor);
    const struct wlr_tablet_v2_event_cursor *event = data;
    struct wlr_surface *focus = tool->v2->focused_surface;
    if (tool->route != TOOL_ROUTE_CLIENT || focus == NULL ||
        event->seat_client->client != wl_resource_get_client(focus->resource)) {
        return;
    }
    cursor_surface surface = {event->surface, event->hotspot_x, event->hotspot_y};
    cursor_image_set_surface(tool->owner->seat->image, &surface);
}

static void remember_position(tablet_tool *tool, const struct wlr_tablet_tool_axis_event *axes)
{
    if (axes->updated_axes & WLR_TABLET_TOOL_AXIS_X) {
        tool->position.x = axes->x;
    }
    if (axes->updated_axes & WLR_TABLET_TOOL_AXIS_Y) {
        tool->position.y = axes->y;
    }
}

static void warp(tablet_tool *tool, const tool_event *event)
{
    seat *s = tool->owner->seat;
    struct wlr_input_device *device = &event->tablet->tablet->base;
    seat_notify_activity(s);
    input_point where = absolute_input_position(s->absolute, device, tool->position);
    wlr_cursor_warp_absolute(s->cursor, device, where.x, where.y);
}

static void release_grab_when_idle(tablet_tool *tool)
{
    if (!tool->v2->is_down && tool->v2->num_buttons == 0) {
        tablet_route_drop_grab(tool);
    }
}

static uint32_t emulated_button(uint32_t stylus_button)
{
    switch (stylus_button) {
    case BTN_STYLUS:
        return BTN_MIDDLE;
    case BTN_STYLUS2:
        return BTN_RIGHT;
    default:
        return stylus_button;
    }
}

static void release_tip(tablet_tool *tool, uint32_t time_ms)
{
    if (!tool->tip_down) {
        return;
    }
    button_input input = {
        .button = BTN_LEFT,
        .pressed = false,
        .time_ms = time_ms,
        .device = tool->tip_device,
    };
    tool->tip_down = false;
    if (tool->tip_in_core) {
        tool->tip_in_core = false;
        tablet_route_tip_to_core(tool, &input);
        return;
    }
    if (tool->route != TOOL_ROUTE_CLIENT) {
        pointer_input_button(tool->owner->seat->pointer, &input);
        return;
    }
    wlr_tablet_v2_tablet_tool_notify_up(tool->v2);
    tablet_route_tip_to_core(tool, &input);
    release_grab_when_idle(tool);
}
