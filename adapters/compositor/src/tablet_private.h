#ifndef COMPOSITOR_TABLET_PRIVATE_H
#define COMPOSITOR_TABLET_PRIVATE_H

#include <wayland-server-core.h>

#include "input_transform.h"
#include "seat_private.h"

struct wlr_tablet;
struct wlr_tablet_tool;
struct wlr_tablet_tool_axis_event;
struct wlr_tablet_pad;
struct wlr_tablet_manager_v2;
struct wlr_tablet_v2_tablet;
struct wlr_tablet_v2_tablet_tool;
struct wlr_tablet_v2_tablet_pad;
struct wlr_scene_node;

typedef enum tool_route {
    TOOL_ROUTE_NONE,
    TOOL_ROUTE_CLIENT,
    TOOL_ROUTE_EMULATED,
} tool_route;

typedef struct tablet_device {
    tablet_input *owner;
    struct wlr_tablet *tablet;
    struct wlr_tablet_v2_tablet *v2;
    struct wl_list link;
    struct wl_listener destroy;
} tablet_device;

typedef struct tablet_tool {
    tablet_input *owner;
    struct wlr_tablet_tool *tool;
    struct wlr_tablet_v2_tablet_tool *v2;
    tool_route route;
    bool tip_down;
    input_point position;
    input_point tilt;
    struct wlr_scene_node *grab_node;
    struct wl_listener grab_node_destroy;
    struct wl_listener destroy;
    struct wl_listener set_cursor;
    struct wl_list link;
} tablet_tool;

typedef struct tool_event {
    tablet_device *tablet;
    uint32_t time_ms;
} tool_event;

struct tablet_input {
    seat *seat;
    struct wlr_tablet_manager_v2 *manager;
    struct wl_list tablets;
    struct wl_list tools;
    struct wl_list pads;
    struct wl_listener new_input;
    struct wl_listener keyboard_focus;
    struct wl_listener axis;
    struct wl_listener proximity;
    struct wl_listener tip;
    struct wl_listener button;
};

tablet_tool *tablet_tool_get(tablet_input *tablets, struct wlr_tablet_tool *tool);
void tablet_tool_free(tablet_tool *tool);
void tablet_tool_enter(tablet_tool *tool, const tool_event *event);
void tablet_tool_leave(tablet_tool *tool);
void tablet_tool_axis(tablet_tool *tool, const tool_event *event,
                      const struct wlr_tablet_tool_axis_event *axes);
void tablet_tool_press_tip(tablet_tool *tool, const tool_event *event);
void tablet_tool_release_tip(tablet_tool *tool, const tool_event *event);
void tablet_tool_button(tablet_tool *tool, const button_input *input);
void tablet_axes_send(tablet_tool *tool, const tool_event *event,
                      const struct wlr_tablet_tool_axis_event *axes);

void tablet_route_update(tablet_tool *tool, const tool_event *event);
void tablet_route_hold_target(tablet_tool *tool);
void tablet_route_drop_grab(tablet_tool *tool);
wm_dispatch tablet_route_tip_to_core(tablet_tool *tool, const button_input *input);

void tablet_pads_add(tablet_input *tablets, struct wlr_input_device *device);
void tablet_pads_destroy(tablet_input *tablets);
void tablet_pads_focus(tablet_input *tablets, struct wlr_surface *surface);
void tablet_pads_tablet_added(tablet_input *tablets);

#endif
