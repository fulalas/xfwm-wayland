#ifndef COMPOSITOR_XWAYLAND_VIEW_PRIVATE_H
#define COMPOSITOR_XWAYLAND_VIEW_PRIVATE_H

#include <wayland-server-core.h>

#include "icon_slot.h"
#include "unmanaged.h"
#include "view.h"
#include "x11_gravity.h"
#include "xwayland_window_props.h"
#include "xwayland_view.h"

typedef struct xwayland_requests {
    struct wl_listener configure;
    struct wl_listener move;
    struct wl_listener resize;
    struct wl_listener minimize;
    struct wl_listener maximize;
    struct wl_listener fullscreen;
    struct wl_listener activate;
    struct wl_listener sticky;
    struct wl_listener shaded;
    struct wl_listener above;
    struct wl_listener below;
    struct wl_listener skip_taskbar;
    struct wl_listener skip_pager;
    struct wl_listener demands_attention;
} xwayland_requests;

enum {
    OLD_STRUT_SIDES = 4,
    REMOTE_HOST_MAX = 256,
};

typedef struct xwayland_properties {
    struct wl_listener title;
    struct wl_listener class;
    struct wl_listener parent;
    struct wl_listener window_type;
    struct wl_listener hints;
    struct wl_listener size_hints;
    struct wl_listener decorations;
    struct wl_listener strut_partial;
    struct wl_listener geometry;
    struct wl_listener opacity;
    struct wl_listener icon;
    struct wl_listener ping_timeout;
    struct wl_listener startup_id;
} xwayland_properties;

struct xwayland_view {
    compositor *server;
    struct wlr_xwayland_surface *xsurface;
    view *view;
    unmanaged *unmanaged;
    icon_slot *icon;
    struct wl_list link;
    xwayland_requests requests;
    xwayland_properties properties;
    struct wl_listener destroy;
    struct wl_listener associate;
    struct wl_listener dissociate;
    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener commit;
    x11_window_props published;
    uint32_t motif_capabilities;
    bool shaped;
    bool has_user_time;
    uint32_t user_time;
    bool opacity_locked;
    bool transient_for_group;
    bool has_old_strut;
    xcb_ewmh_wm_strut_partial_t old_strut;
    char remote_host[REMOTE_HOST_MAX];
};

void xwayland_requests_connect(xwayland_view *view);
void xwayland_requests_disconnect(xwayland_view *view);
void xwayland_properties_connect(xwayland_view *view);
void xwayland_properties_disconnect(xwayland_view *view);
wm_window *xwayland_view_window(const xwayland_view *view);
wm_window_desc xwayland_view_describe(const xwayland_view *view);
uint32_t xwayland_view_capabilities(const xwayland_view *view);
void xwayland_view_read_properties(xwayland_view *view);
void xwayland_view_read_motif(xwayland_view *view);
char *xwayland_view_remote_title(const xwayland_view *view);
bool xwayland_view_has_strut(const xwayland_view *view);
wm_window_type xwayland_view_type(const xwayland_view *view);
wm_size_hints xwayland_view_size_hints(const xwayland_view *view);
wm_window *xwayland_view_parent_window(const xwayland_view *view);
bool xwayland_view_is_urgent(const xwayland_view *view);
void xwayland_view_report_urgency(xwayland_view *view);
void xwayland_view_fetch_icon(xwayland_view *view);
strut xwayland_view_strut(const xwayland_view *view);
void xwayland_view_send_states(void *self, const wm_window_view *state);
void xwayland_view_settle_if_current(xwayland_view *view);
int xwayland_view_gravity(const xwayland_view *view);
wm_rect xwayland_view_requested_rect(const xwayland_view *view, const x11_configure *request);
wm_point xwayland_view_initial_position(const xwayland_view *view);

#endif
