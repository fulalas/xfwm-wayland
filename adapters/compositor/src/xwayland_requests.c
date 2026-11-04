#include <wlr/xwayland/xwayland.h>

#include "input_mapping.h"
#include "listener.h"
#include "xwayland_view_private.h"

typedef struct flag_request {
    uint32_t state;
    bool value;
} flag_request;

static void handle_configure(struct wl_listener *listener, void *data);
static void handle_move(struct wl_listener *listener, void *data);
static void handle_resize(struct wl_listener *listener, void *data);
static void handle_minimize(struct wl_listener *listener, void *data);
static void handle_maximize(struct wl_listener *listener, void *data);
static void handle_fullscreen(struct wl_listener *listener, void *data);
static void handle_activate(struct wl_listener *listener, void *data);
static void handle_sticky(struct wl_listener *listener, void *data);
static void handle_shaded(struct wl_listener *listener, void *data);
static void handle_above(struct wl_listener *listener, void *data);
static void handle_below(struct wl_listener *listener, void *data);
static void handle_skip_taskbar(struct wl_listener *listener, void *data);
static void handle_skip_pager(struct wl_listener *listener, void *data);
static void handle_demands_attention(struct wl_listener *listener, void *data);
static void request_flag(xwayland_view *view, flag_request request);
static void request_state(xwayland_view *view, const wm_state_change *change);

void xwayland_requests_connect(xwayland_view *xv)
{
    struct wlr_xwayland_surface *xs = xv->xsurface;
    xwayland_requests *r = &xv->requests;
    listener_connect(&r->configure, &xs->events.request_configure, handle_configure);
    listener_connect(&r->move, &xs->events.request_move, handle_move);
    listener_connect(&r->resize, &xs->events.request_resize, handle_resize);
    listener_connect(&r->minimize, &xs->events.request_minimize, handle_minimize);
    listener_connect(&r->maximize, &xs->events.request_maximize, handle_maximize);
    listener_connect(&r->fullscreen, &xs->events.request_fullscreen, handle_fullscreen);
    listener_connect(&r->activate, &xs->events.request_activate, handle_activate);
    listener_connect(&r->sticky, &xs->events.request_sticky, handle_sticky);
    listener_connect(&r->shaded, &xs->events.request_shaded, handle_shaded);
    listener_connect(&r->above, &xs->events.request_above, handle_above);
    listener_connect(&r->below, &xs->events.request_below, handle_below);
    listener_connect(&r->skip_taskbar, &xs->events.request_skip_taskbar, handle_skip_taskbar);
    listener_connect(&r->skip_pager, &xs->events.request_skip_pager, handle_skip_pager);
    listener_connect(&r->demands_attention, &xs->events.request_demands_attention,
                     handle_demands_attention);
}

void xwayland_requests_disconnect(xwayland_view *xv)
{
    xwayland_requests *r = &xv->requests;
    struct wl_listener *all[] = {
        &r->configure, &r->move,  &r->resize, &r->minimize,     &r->maximize,
        &r->fullscreen, &r->activate, &r->sticky, &r->shaded,   &r->above,
        &r->below,     &r->skip_taskbar, &r->skip_pager, &r->demands_attention,
    };
    for (size_t i = 0; i < sizeof(all) / sizeof(all[0]); i++) {
        listener_disconnect(all[i]);
    }
}

void xwayland_view_report_urgency(xwayland_view *xv)
{
    wm_window *window = xwayland_view_window(xv);
    if (window == NULL) {
        return;
    }
    if (xwayland_view_is_urgent(xv)) {
        wm_window_demand_attention(xv->server->desktop, window);
    } else {
        wm_window_clear_attention(xv->server->desktop, window);
    }
}

static void handle_configure(struct wl_listener *listener, void *data)
{
    xwayland_view *xv = wl_container_of(listener, xv, requests.configure);
    const struct wlr_xwayland_surface_configure_event *event = data;
    wm_window *window = xwayland_view_window(xv);
    if (window != NULL) {
        wm_rect client = {event->x, event->y, event->width, event->height};
        wm_request_configure(xv->server->desktop, window, &client);
        return;
    }
    wlr_xwayland_surface_configure(xv->xsurface, event->x, event->y, event->width,
                                   event->height);
}

static void handle_move(struct wl_listener *listener, void *data)
{
    (void) data;
    xwayland_view *xv = wl_container_of(listener, xv, requests.move);
    wm_window *window = xwayland_view_window(xv);
    if (window != NULL) {
        wm_request_move(xv->server->desktop, window);
    }
}

static void handle_resize(struct wl_listener *listener, void *data)
{
    xwayland_view *xv = wl_container_of(listener, xv, requests.resize);
    const struct wlr_xwayland_resize_event *event = data;
    wm_window *window = xwayland_view_window(xv);
    if (window != NULL) {
        uint32_t edges = input_mapping_edges_from_wlr(event->edges);
        wm_request_resize(xv->server->desktop, window, edges);
    }
}

static void handle_minimize(struct wl_listener *listener, void *data)
{
    xwayland_view *xv = wl_container_of(listener, xv, requests.minimize);
    const struct wlr_xwayland_minimize_event *event = data;
    request_flag(xv, (flag_request){WM_STATE_MINIMIZED, event->minimize});
}

static void handle_maximize(struct wl_listener *listener, void *data)
{
    (void) data;
    xwayland_view *xv = wl_container_of(listener, xv, requests.maximize);
    uint32_t wanted = 0;
    wanted |= xv->xsurface->maximized_horz ? WM_STATE_MAXIMIZED_HORZ : 0;
    wanted |= xv->xsurface->maximized_vert ? WM_STATE_MAXIMIZED_VERT : 0;
    wm_state_change change = {wanted, WM_STATE_MAXIMIZED & ~wanted};
    request_state(xv, &change);
}

static void handle_fullscreen(struct wl_listener *listener, void *data)
{
    (void) data;
    xwayland_view *xv = wl_container_of(listener, xv, requests.fullscreen);
    request_flag(xv, (flag_request){WM_STATE_FULLSCREEN, xv->xsurface->fullscreen});
}

static void handle_activate(struct wl_listener *listener, void *data)
{
    (void) data;
    xwayland_view *xv = wl_container_of(listener, xv, requests.activate);
    wm_window *window = xwayland_view_window(xv);
    if (window != NULL) {
        wm_request_activate(xv->server->desktop, window, WM_ACTIVATION_APPLICATION);
    }
}

static void handle_sticky(struct wl_listener *listener, void *data)
{
    (void) data;
    xwayland_view *xv = wl_container_of(listener, xv, requests.sticky);
    request_flag(xv, (flag_request){WM_STATE_STICKY, xv->xsurface->sticky});
}

static void handle_shaded(struct wl_listener *listener, void *data)
{
    (void) data;
    xwayland_view *xv = wl_container_of(listener, xv, requests.shaded);
    request_flag(xv, (flag_request){WM_STATE_SHADED, xv->xsurface->shaded});
}

static void handle_above(struct wl_listener *listener, void *data)
{
    (void) data;
    xwayland_view *xv = wl_container_of(listener, xv, requests.above);
    request_flag(xv, (flag_request){WM_STATE_ABOVE, xv->xsurface->above});
}

static void handle_below(struct wl_listener *listener, void *data)
{
    (void) data;
    xwayland_view *xv = wl_container_of(listener, xv, requests.below);
    request_flag(xv, (flag_request){WM_STATE_BELOW, xv->xsurface->below});
}

static void handle_skip_taskbar(struct wl_listener *listener, void *data)
{
    (void) data;
    xwayland_view *xv = wl_container_of(listener, xv, requests.skip_taskbar);
    request_flag(xv, (flag_request){WM_STATE_SKIP_TASKBAR, xv->xsurface->skip_taskbar});
}

static void handle_skip_pager(struct wl_listener *listener, void *data)
{
    (void) data;
    xwayland_view *xv = wl_container_of(listener, xv, requests.skip_pager);
    request_flag(xv, (flag_request){WM_STATE_SKIP_PAGER, xv->xsurface->skip_pager});
}

static void handle_demands_attention(struct wl_listener *listener, void *data)
{
    (void) data;
    xwayland_view *xv = wl_container_of(listener, xv, requests.demands_attention);
    xwayland_view_report_urgency(xv);
}

static void request_flag(xwayland_view *xv, flag_request request)
{
    wm_state_change change = view_state_change(request.state, request.value);
    request_state(xv, &change);
}

static void request_state(xwayland_view *xv, const wm_state_change *change)
{
    if (xv->view != NULL) {
        view_request_state(xv->view, change);
    }
}
