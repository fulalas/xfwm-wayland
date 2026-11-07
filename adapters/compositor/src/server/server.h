#ifndef COMPOSITOR_SERVER_H
#define COMPOSITOR_SERVER_H

#include <stdbool.h>
#include <wayland-server-core.h>

#include "compositor/compositor.h"

typedef struct scene_layers scene_layers;
typedef struct outputs outputs;
typedef struct seat seat;
typedef struct xdg_shell xdg_shell;
typedef struct layer_shell layer_shell;
typedef struct xwayland_shell xwayland_shell;
typedef struct session_lock session_lock;
typedef struct osd osd;
typedef struct zoom zoom;
typedef struct shadow_cache shadow_cache;
typedef struct toplevel_icons toplevel_icons;
typedef struct idle idle;
typedef struct foreign_toplevels foreign_toplevels;
typedef struct toplevel_capture toplevel_capture;
typedef struct workspace_protocol workspace_protocol;
typedef struct activation activation;
typedef struct output_config output_config;
typedef struct output_power output_power;
typedef struct displays displays;
typedef struct lid_switch lid_switch;
typedef struct xfce_outputs xfce_outputs;
typedef struct xfce_output_heads xfce_output_heads;
typedef struct accessibility accessibility;
typedef struct device_list device_list;
typedef struct drm_lease drm_lease;
typedef struct xdg_sessions xdg_sessions;
typedef struct present_sync present_sync;

typedef struct protocol_globals {
    struct wlr_compositor *compositor;
    struct wlr_idle_notifier_v1 *idle_notifier;
    struct wlr_idle_inhibit_manager_v1 *idle_inhibit;
    struct wlr_relative_pointer_manager_v1 *relative_pointer;
    struct wlr_pointer_gestures_v1 *gestures;
    struct wlr_pointer_constraints_v1 *constraints;
    struct wlr_keyboard_shortcuts_inhibit_manager_v1 *shortcuts_inhibit;
    struct wlr_cursor_shape_manager_v1 *cursor_shape;
    struct wlr_virtual_keyboard_manager_v1 *virtual_keyboard;
    struct wlr_virtual_pointer_manager_v1 *virtual_pointer;
    struct wlr_tearing_control_manager_v1 *tearing;
    struct wlr_xdg_wm_dialog_v1 *dialog;
    struct wlr_xdg_toplevel_icon_manager_v1 *toplevel_icon;
    struct wlr_xdg_decoration_manager_v1 *xdg_decoration;
    struct wlr_server_decoration_manager *server_decoration;
    struct wlr_security_context_manager_v1 *security_context;
    struct wlr_color_manager_v1 *color_manager;
    struct wlr_foreign_toplevel_manager_v1 *foreign_toplevel;
    struct wlr_ext_foreign_toplevel_list_v1 *foreign_toplevel_list;
    struct wlr_ext_workspace_manager_v1 *workspace;
    struct wlr_output_manager_v1 *output_manager;
    struct wlr_output_power_manager_v1 *output_power;
    struct wlr_ext_foreign_toplevel_image_capture_source_manager_v1 *toplevel_capture;
} protocol_globals;

typedef struct view_signals {
    struct wl_signal mapped;
    struct wl_signal changed;
    struct wl_signal unmapped;
} view_signals;

typedef enum renderer_kind {
    RENDERER_KIND_NONE,
    RENDERER_KIND_DEFAULT,
    RENDERER_KIND_VULKAN,
    RENDERER_KIND_PIXMAN,
} renderer_kind;

typedef struct renderer_state {
    renderer_kind current;
    renderer_kind failed;
    bool forced;
    struct wl_listener lost;
} renderer_state;

struct compositor {
    const compositor_painter *painter;
    const compositor_services *services;
    const compositor_listener *listener;
    wm_config config;
    wm_desktop *desktop;
    struct wl_display *display;
    struct wl_event_loop *loop;
    struct wlr_backend *backend;
    struct wlr_session *session;
    struct wlr_renderer *renderer;
    struct wlr_allocator *allocator;
    renderer_state render;
    struct wlr_scene *scene;
    struct wlr_output_layout *layout;
    struct wlr_scene_output_layout *scene_layout;
    protocol_globals globals;
    scene_layers *layers;
    outputs *outputs;
    seat *seat;
    xdg_shell *xdg;
    layer_shell *layer_shell;
    xwayland_shell *xwayland;
    session_lock *lock;
    osd *osd;
    zoom *zoom;
    shadow_cache *shadows;
    toplevel_icons *toplevel_icons;
    idle *idle;
    foreign_toplevels *foreign_toplevels;
    toplevel_capture *toplevel_capture;
    workspace_protocol *workspaces;
    activation *activation;
    output_config *output_config;
    output_power *output_power;
    displays *displays;
    lid_switch *lid_switch;
    xfce_outputs *xfce_outputs;
    xfce_output_heads *xfce_output_heads;
    accessibility *accessibility;
    device_list *device_list;
    drm_lease *drm_lease;
    xdg_sessions *xdg_sessions;
    present_sync *present_sync;
    struct wl_list views;
    view_signals view_events;
    struct wl_event_source *timer;
    struct wl_listener backend_destroy;
    const char *socket;
    bool scanout_allowed;
};

#endif
