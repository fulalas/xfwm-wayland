#include <linux/input-event-codes.h>
#include <math.h>
#include <stdlib.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_tablet_tool.h>

#include "listener.h"
#include "seat_private.h"

struct tablet_input {
    seat *seat;
    struct wl_listener axis;
    struct wl_listener proximity;
    struct wl_listener tip;
    struct wl_listener button;
};

static void handle_axis(struct wl_listener *listener, void *data);
static void handle_proximity(struct wl_listener *listener, void *data);
static void handle_tip(struct wl_listener *listener, void *data);
static void handle_button(struct wl_listener *listener, void *data);
static void warp_to(tablet_input *tablet, struct wlr_tablet *device, layout_point_fraction where);

tablet_input *tablet_input_create(seat *s)
{
    tablet_input *tablet = calloc(1, sizeof(*tablet));
    if (tablet == NULL) {
        return NULL;
    }
    tablet->seat = s;
    listener_connect(&tablet->axis, &s->cursor->events.tablet_tool_axis, handle_axis);
    listener_connect(&tablet->proximity, &s->cursor->events.tablet_tool_proximity,
                     handle_proximity);
    listener_connect(&tablet->tip, &s->cursor->events.tablet_tool_tip, handle_tip);
    listener_connect(&tablet->button, &s->cursor->events.tablet_tool_button, handle_button);
    return tablet;
}

void tablet_input_destroy(tablet_input *tablet)
{
    listener_disconnect(&tablet->axis);
    listener_disconnect(&tablet->proximity);
    listener_disconnect(&tablet->tip);
    listener_disconnect(&tablet->button);
    free(tablet);
}

static void handle_axis(struct wl_listener *listener, void *data)
{
    tablet_input *tablet = wl_container_of(listener, tablet, axis);
    const struct wlr_tablet_tool_axis_event *event = data;
    bool has_x = (event->updated_axes & WLR_TABLET_TOOL_AXIS_X) != 0;
    bool has_y = (event->updated_axes & WLR_TABLET_TOOL_AXIS_Y) != 0;
    if (!has_x && !has_y) {
        return;
    }
    layout_point_fraction where = {has_x ? event->x : NAN, has_y ? event->y : NAN};
    warp_to(tablet, event->tablet, where);
    pointer_input_motion(tablet->seat->pointer, event->time_msec);
}

static void handle_proximity(struct wl_listener *listener, void *data)
{
    tablet_input *tablet = wl_container_of(listener, tablet, proximity);
    const struct wlr_tablet_tool_proximity_event *event = data;
    if (event->state != WLR_TABLET_TOOL_PROXIMITY_IN) {
        return;
    }
    warp_to(tablet, event->tablet, (layout_point_fraction){event->x, event->y});
    pointer_input_motion(tablet->seat->pointer, event->time_msec);
}

static void handle_tip(struct wl_listener *listener, void *data)
{
    tablet_input *tablet = wl_container_of(listener, tablet, tip);
    const struct wlr_tablet_tool_tip_event *event = data;
    button_input input = {
        .button = BTN_LEFT,
        .pressed = event->state == WLR_TABLET_TOOL_TIP_DOWN,
        .time_ms = event->time_msec,
    };
    pointer_input_button(tablet->seat->pointer, &input);
}

static void handle_button(struct wl_listener *listener, void *data)
{
    tablet_input *tablet = wl_container_of(listener, tablet, button);
    const struct wlr_tablet_tool_button_event *event = data;
    button_input input = {
        .button = event->button,
        .pressed = event->state == WLR_BUTTON_PRESSED,
        .time_ms = event->time_msec,
    };
    pointer_input_button(tablet->seat->pointer, &input);
}

static void warp_to(tablet_input *tablet, struct wlr_tablet *device, layout_point_fraction where)
{
    seat_notify_activity(tablet->seat);
    wlr_cursor_warp_absolute(tablet->seat->cursor, &device->base, where.x, where.y);
}
