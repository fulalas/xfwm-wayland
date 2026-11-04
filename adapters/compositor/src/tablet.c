#include <stdlib.h>
#include <wlr/backend.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_tablet_tool.h>
#include <wlr/types/wlr_tablet_v2.h>
#include <wlr/util/log.h>

#include "listener.h"
#include "tablet_private.h"

static void handle_new_input(struct wl_listener *listener, void *data);
static void handle_keyboard_focus(struct wl_listener *listener, void *data);
static void handle_axis(struct wl_listener *listener, void *data);
static void handle_proximity(struct wl_listener *listener, void *data);
static void handle_tip(struct wl_listener *listener, void *data);
static void handle_button(struct wl_listener *listener, void *data);
static void handle_tablet_destroy(struct wl_listener *listener, void *data);
static void add_tablet(tablet_input *tablets, struct wlr_input_device *device);
static void free_tablet(tablet_device *tablet);
static void connect_cursor(tablet_input *tablets, struct wlr_cursor *cursor);
static tablet_device *find_tablet(tablet_input *tablets, struct wlr_tablet *device);

tablet_input *tablet_input_create(seat *s)
{
    tablet_input *tablets = calloc(1, sizeof(*tablets));
    if (tablets == NULL) {
        return NULL;
    }
    tablets->seat = s;
    wl_list_init(&tablets->tablets);
    wl_list_init(&tablets->tools);
    wl_list_init(&tablets->pads);
    tablets->manager = wlr_tablet_v2_create(s->server->display);
    if (tablets->manager == NULL) {
        wlr_log(WLR_ERROR, "cannot create the tablet global");
        free(tablets);
        return NULL;
    }
    listener_connect(&tablets->new_input, &s->server->backend->events.new_input,
                     handle_new_input);
    listener_connect(&tablets->keyboard_focus, &s->wlr_seat->keyboard_state.events.focus_change,
                     handle_keyboard_focus);
    connect_cursor(tablets, s->cursor);
    return tablets;
}

void tablet_input_destroy(tablet_input *tablets)
{
    listener_disconnect(&tablets->new_input);
    listener_disconnect(&tablets->keyboard_focus);
    listener_disconnect(&tablets->axis);
    listener_disconnect(&tablets->proximity);
    listener_disconnect(&tablets->tip);
    listener_disconnect(&tablets->button);
    tablet_pads_destroy(tablets);
    tablet_tool *tool;
    tablet_tool *next_tool;
    wl_list_for_each_safe(tool, next_tool, &tablets->tools, link) {
        tablet_tool_free(tool);
    }
    tablet_device *tablet;
    tablet_device *next_tablet;
    wl_list_for_each_safe(tablet, next_tablet, &tablets->tablets, link) {
        free_tablet(tablet);
    }
    free(tablets);
}

static void handle_new_input(struct wl_listener *listener, void *data)
{
    tablet_input *tablets = wl_container_of(listener, tablets, new_input);
    struct wlr_input_device *device = data;
    if (device->type == WLR_INPUT_DEVICE_TABLET) {
        add_tablet(tablets, device);
    } else if (device->type == WLR_INPUT_DEVICE_TABLET_PAD) {
        tablet_pads_add(tablets, device);
    }
}

static void handle_keyboard_focus(struct wl_listener *listener, void *data)
{
    tablet_input *tablets = wl_container_of(listener, tablets, keyboard_focus);
    const struct wlr_seat_keyboard_focus_change_event *event = data;
    tablet_pads_focus(tablets, event->new_surface);
}

static void handle_axis(struct wl_listener *listener, void *data)
{
    tablet_input *tablets = wl_container_of(listener, tablets, axis);
    const struct wlr_tablet_tool_axis_event *event = data;
    tool_event context = {find_tablet(tablets, event->tablet), event->time_msec};
    tablet_tool *tool = tablet_tool_get(tablets, event->tool);
    if (tool != NULL && context.tablet != NULL) {
        tablet_tool_axis(tool, &context, event);
    }
}

static void handle_proximity(struct wl_listener *listener, void *data)
{
    tablet_input *tablets = wl_container_of(listener, tablets, proximity);
    const struct wlr_tablet_tool_proximity_event *event = data;
    tool_event context = {find_tablet(tablets, event->tablet), event->time_msec};
    tablet_tool *tool = tablet_tool_get(tablets, event->tool);
    if (tool == NULL || context.tablet == NULL) {
        return;
    }
    if (event->state == WLR_TABLET_TOOL_PROXIMITY_OUT) {
        tablet_tool_leave(tool);
        return;
    }
    tool->position = (input_point){event->x, event->y};
    tablet_tool_enter(tool, &context);
}

static void handle_tip(struct wl_listener *listener, void *data)
{
    tablet_input *tablets = wl_container_of(listener, tablets, tip);
    const struct wlr_tablet_tool_tip_event *event = data;
    tool_event context = {find_tablet(tablets, event->tablet), event->time_msec};
    tablet_tool *tool = tablet_tool_get(tablets, event->tool);
    if (tool == NULL || context.tablet == NULL) {
        return;
    }
    if (event->state == WLR_TABLET_TOOL_TIP_DOWN) {
        tablet_tool_press_tip(tool, &context);
    } else {
        tablet_tool_release_tip(tool, &context);
    }
}

static void handle_button(struct wl_listener *listener, void *data)
{
    tablet_input *tablets = wl_container_of(listener, tablets, button);
    const struct wlr_tablet_tool_button_event *event = data;
    tablet_tool *tool = tablet_tool_get(tablets, event->tool);
    if (tool == NULL) {
        return;
    }
    button_input input = {
        .button = event->button,
        .pressed = event->state == WLR_BUTTON_PRESSED,
        .time_ms = event->time_msec,
    };
    seat_notify_activity(tablets->seat);
    tablet_tool_button(tool, &input);
}

static void handle_tablet_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    tablet_device *tablet = wl_container_of(listener, tablet, destroy);
    free_tablet(tablet);
}

static void add_tablet(tablet_input *tablets, struct wlr_input_device *device)
{
    tablet_device *tablet = calloc(1, sizeof(*tablet));
    if (tablet == NULL) {
        return;
    }
    tablet->owner = tablets;
    tablet->tablet = wlr_tablet_from_input_device(device);
    tablet->v2 = wlr_tablet_create(tablets->manager, tablets->seat->wlr_seat, device);
    listener_connect(&tablet->destroy, &device->events.destroy, handle_tablet_destroy);
    wl_list_insert(&tablets->tablets, &tablet->link);
    tablet_pads_tablet_added(tablets);
}

static void free_tablet(tablet_device *tablet)
{
    listener_disconnect(&tablet->destroy);
    wl_list_remove(&tablet->link);
    free(tablet);
}

static void connect_cursor(tablet_input *tablets, struct wlr_cursor *cursor)
{
    listener_connect(&tablets->axis, &cursor->events.tablet_tool_axis, handle_axis);
    listener_connect(&tablets->proximity, &cursor->events.tablet_tool_proximity,
                     handle_proximity);
    listener_connect(&tablets->tip, &cursor->events.tablet_tool_tip, handle_tip);
    listener_connect(&tablets->button, &cursor->events.tablet_tool_button, handle_button);
}

static tablet_device *find_tablet(tablet_input *tablets, struct wlr_tablet *device)
{
    tablet_device *tablet;
    wl_list_for_each(tablet, &tablets->tablets, link) {
        if (tablet->tablet == device) {
            return tablet;
        }
    }
    return NULL;
}
