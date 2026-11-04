#include <wlr/types/wlr_xdg_shell.h>

#include "input_mapping.h"
#include "listener.h"
#include "xdg_view_private.h"

static void handle_maximize(struct wl_listener *listener, void *data);
static void handle_fullscreen(struct wl_listener *listener, void *data);
static void handle_minimize(struct wl_listener *listener, void *data);
static void handle_move(struct wl_listener *listener, void *data);
static void handle_resize(struct wl_listener *listener, void *data);
static void handle_show_window_menu(struct wl_listener *listener, void *data);
static void handle_set_title(struct wl_listener *listener, void *data);
static void handle_set_app_id(struct wl_listener *listener, void *data);
static void handle_set_parent(struct wl_listener *listener, void *data);
static void handle_ping_timeout(struct wl_listener *listener, void *data);
static void request_state(xdg_view *view, wm_state_change change);
static wm_window *mapped_window(const xdg_view *view);

void xdg_requests_connect(xdg_view *xv)
{
    struct wlr_xdg_toplevel *toplevel = xv->toplevel;
    xdg_requests *r = &xv->requests;
    listener_connect(&r->maximize, &toplevel->events.request_maximize, handle_maximize);
    listener_connect(&r->fullscreen, &toplevel->events.request_fullscreen, handle_fullscreen);
    listener_connect(&r->minimize, &toplevel->events.request_minimize, handle_minimize);
    listener_connect(&r->move, &toplevel->events.request_move, handle_move);
    listener_connect(&r->resize, &toplevel->events.request_resize, handle_resize);
    listener_connect(&r->show_window_menu, &toplevel->events.request_show_window_menu,
                     handle_show_window_menu);
    listener_connect(&r->set_title, &toplevel->events.set_title, handle_set_title);
    listener_connect(&r->set_app_id, &toplevel->events.set_app_id, handle_set_app_id);
    listener_connect(&r->set_parent, &toplevel->events.set_parent, handle_set_parent);
    listener_connect(&r->ping_timeout, &toplevel->base->events.ping_timeout, handle_ping_timeout);
}

void xdg_requests_disconnect(xdg_view *xv)
{
    xdg_requests *r = &xv->requests;
    listener_disconnect(&r->maximize);
    listener_disconnect(&r->fullscreen);
    listener_disconnect(&r->minimize);
    listener_disconnect(&r->move);
    listener_disconnect(&r->resize);
    listener_disconnect(&r->show_window_menu);
    listener_disconnect(&r->set_title);
    listener_disconnect(&r->set_app_id);
    listener_disconnect(&r->set_parent);
    listener_disconnect(&r->ping_timeout);
}

static void handle_maximize(struct wl_listener *listener, void *data)
{
    (void) data;
    xdg_view *xv = wl_container_of(listener, xv, requests.maximize);
    bool wanted = xv->toplevel->requested.maximized;
    request_state(xv, view_state_change(WM_STATE_MAXIMIZED, wanted));
}

static void handle_fullscreen(struct wl_listener *listener, void *data)
{
    (void) data;
    xdg_view *xv = wl_container_of(listener, xv, requests.fullscreen);
    bool wanted = xv->toplevel->requested.fullscreen;
    request_state(xv, view_state_change(WM_STATE_FULLSCREEN, wanted));
}

static void handle_minimize(struct wl_listener *listener, void *data)
{
    (void) data;
    xdg_view *xv = wl_container_of(listener, xv, requests.minimize);
    if (xv->toplevel->requested.minimized) {
        request_state(xv, view_state_change(WM_STATE_MINIMIZED, true));
    }
}

static void handle_move(struct wl_listener *listener, void *data)
{
    (void) data;
    xdg_view *xv = wl_container_of(listener, xv, requests.move);
    wm_window *window = mapped_window(xv);
    if (window != NULL) {
        wm_request_move(xv->server->desktop, window);
    }
}

static void handle_resize(struct wl_listener *listener, void *data)
{
    xdg_view *xv = wl_container_of(listener, xv, requests.resize);
    const struct wlr_xdg_toplevel_resize_event *event = data;
    wm_window *window = mapped_window(xv);
    if (window != NULL) {
        uint32_t edges = input_mapping_edges_from_wlr(event->edges);
        wm_request_resize(xv->server->desktop, window, edges);
    }
}

static void handle_show_window_menu(struct wl_listener *listener, void *data)
{
    xdg_view *xv = wl_container_of(listener, xv, requests.show_window_menu);
    const struct wlr_xdg_toplevel_show_window_menu_event *event = data;
    wm_window *window = mapped_window(xv);
    if (window != NULL) {
        wm_point point = {event->x, event->y};
        wm_request_window_menu(xv->server->desktop, window, point);
    }
}

static void handle_set_title(struct wl_listener *listener, void *data)
{
    (void) data;
    xdg_view *xv = wl_container_of(listener, xv, requests.set_title);
    wm_window *window = mapped_window(xv);
    if (window != NULL) {
        wm_window_set_title(xv->server->desktop, window, xv->toplevel->title);
    }
}

static void handle_set_app_id(struct wl_listener *listener, void *data)
{
    (void) data;
    xdg_view *xv = wl_container_of(listener, xv, requests.set_app_id);
    wm_window *window = mapped_window(xv);
    if (window != NULL) {
        wm_window_set_app_id(xv->server->desktop, window, xv->toplevel->app_id);
    }
    icon_slot_lookup_app(xv->icon, xv->toplevel->app_id);
}

static void handle_set_parent(struct wl_listener *listener, void *data)
{
    (void) data;
    xdg_view *xv = wl_container_of(listener, xv, requests.set_parent);
    wm_window *window = mapped_window(xv);
    if (window != NULL) {
        wm_window_set_parent(xv->server->desktop, window, xdg_view_parent_window(xv));
    }
}

static void handle_ping_timeout(struct wl_listener *listener, void *data)
{
    (void) data;
    xdg_view *xv = wl_container_of(listener, xv, requests.ping_timeout);
    if (xv->view != NULL) {
        view_report_not_responding(xv->view);
    }
}

static void request_state(xdg_view *xv, wm_state_change change)
{
    if (xv->view != NULL) {
        view_request_state(xv->view, &change);
        return;
    }
    if (xv->toplevel->base->initialized) {
        wlr_xdg_surface_schedule_configure(xv->toplevel->base);
    }
}

static wm_window *mapped_window(const xdg_view *xv)
{
    return xv->view != NULL ? view_window(xv->view) : NULL;
}
