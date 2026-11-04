#ifndef COMPOSITOR_XDG_VIEW_PRIVATE_H
#define COMPOSITOR_XDG_VIEW_PRIVATE_H

#include <wayland-server-core.h>
#include <wlr/util/addon.h>

#include "popup.h"
#include "view.h"
#include "xdg_view.h"

typedef enum decoration_mode {
    DECORATION_CLIENT,
    DECORATION_SERVER,
} decoration_mode;

typedef struct xdg_decorations {
    struct wlr_xdg_toplevel_decoration_v1 *xdg;
    struct wlr_server_decoration *kde;
    decoration_mode mode;
    struct wl_listener xdg_request_mode;
    struct wl_listener xdg_destroy;
    struct wl_listener kde_mode;
    struct wl_listener kde_destroy;
} xdg_decorations;

typedef struct xdg_requests {
    struct wl_listener maximize;
    struct wl_listener fullscreen;
    struct wl_listener minimize;
    struct wl_listener move;
    struct wl_listener resize;
    struct wl_listener show_window_menu;
    struct wl_listener set_title;
    struct wl_listener set_app_id;
    struct wl_listener set_parent;
    struct wl_listener ping_timeout;
} xdg_requests;

struct xdg_view {
    compositor *server;
    struct wlr_xdg_toplevel *toplevel;
    view *view;
    icon_slot *icon;
    popup_parent popup_parent;
    uint32_t pending_serial;
    wm_size_hints size_hints;
    wm_size deferred_size;
    bool has_deferred_size;
    xdg_decorations decorations;
    xdg_requests requests;
    struct wlr_addon addon;
    struct wl_listener commit;
    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener destroy;
};

void xdg_requests_connect(xdg_view *view);
void xdg_requests_disconnect(xdg_view *view);
void xdg_decorations_init(xdg_view *view);
void xdg_decorations_finish(xdg_view *view);
void xdg_decorations_send_mode(xdg_view *view);
wm_window_desc xdg_view_describe(const xdg_view *view);
uint32_t xdg_view_capabilities(const xdg_view *view);
wm_size_hints xdg_view_size_hints(const xdg_view *view);
wm_window *xdg_view_parent_window(const xdg_view *view);
void xdg_view_send_states(void *self, const wm_window_view *state);

#endif
