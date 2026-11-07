#include <wlr/types/wlr_tablet_tool.h>
#include <wlr/types/wlr_tablet_v2.h>

#include "absolute_input.h"
#include "tablet_private.h"

static void send_tilt(tablet_tool *tool, const tool_event *event,
                      const struct wlr_tablet_tool_axis_event *axes);

void tablet_axes_send(tablet_tool *tool, const tool_event *event,
                      const struct wlr_tablet_tool_axis_event *axes)
{
    if (tool->route != TOOL_ROUTE_CLIENT) {
        return;
    }
    uint32_t updated = axes->updated_axes;
    if (updated & WLR_TABLET_TOOL_AXIS_PRESSURE) {
        wlr_tablet_v2_tablet_tool_notify_pressure(tool->v2, axes->pressure);
    }
    if (updated & WLR_TABLET_TOOL_AXIS_DISTANCE) {
        wlr_tablet_v2_tablet_tool_notify_distance(tool->v2, axes->distance);
    }
    send_tilt(tool, event, axes);
    if (updated & WLR_TABLET_TOOL_AXIS_ROTATION) {
        wlr_tablet_v2_tablet_tool_notify_rotation(tool->v2, axes->rotation);
    }
    if (updated & WLR_TABLET_TOOL_AXIS_SLIDER) {
        wlr_tablet_v2_tablet_tool_notify_slider(tool->v2, axes->slider);
    }
    if (updated & WLR_TABLET_TOOL_AXIS_WHEEL) {
        wlr_tablet_v2_tablet_tool_notify_wheel(tool->v2, axes->wheel_delta, 0);
    }
}

static void send_tilt(tablet_tool *tool, const tool_event *event,
                      const struct wlr_tablet_tool_axis_event *axes)
{
    uint32_t updated = axes->updated_axes;
    if ((updated & (WLR_TABLET_TOOL_AXIS_TILT_X | WLR_TABLET_TOOL_AXIS_TILT_Y)) == 0) {
        return;
    }
    if (updated & WLR_TABLET_TOOL_AXIS_TILT_X) {
        tool->tilt.x = axes->tilt_x;
    }
    if (updated & WLR_TABLET_TOOL_AXIS_TILT_Y) {
        tool->tilt.y = axes->tilt_y;
    }
    seat *s = tool->owner->seat;
    input_point tilt =
        absolute_input_tilt(s->absolute, &event->tablet->tablet->base, tool->tilt);
    wlr_tablet_v2_tablet_tool_notify_tilt(tool->v2, tilt.x, tilt.y);
}
