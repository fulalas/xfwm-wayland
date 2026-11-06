#include <wlr/types/wlr_server_decoration.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>
#include <wlr/types/wlr_xdg_shell.h>

#include "listener.h"
#include "xdg_view_private.h"

static void handle_xdg_request_mode(struct wl_listener *listener, void *data);
static void handle_xdg_destroy(struct wl_listener *listener, void *data);
static void handle_kde_mode(struct wl_listener *listener, void *data);
static void handle_kde_destroy(struct wl_listener *listener, void *data);
static void detach_xdg(xdg_decorations *decorations);
static void detach_kde(xdg_decorations *decorations);
static void refresh_mode(xdg_view *view);
static decoration_mode wanted_mode(const xdg_decorations *decorations);

void xdg_decorations_init(xdg_view *xv)
{
    xdg_decorations *d = &xv->decorations;
    d->mode = DECORATION_CLIENT;
    wl_list_init(&d->xdg_request_mode.link);
    wl_list_init(&d->xdg_destroy.link);
    wl_list_init(&d->kde_mode.link);
    wl_list_init(&d->kde_destroy.link);
}

void xdg_decorations_finish(xdg_view *xv)
{
    detach_xdg(&xv->decorations);
    detach_kde(&xv->decorations);
}

void xdg_view_attach_decoration(xdg_view *xv, struct wlr_xdg_toplevel_decoration_v1 *decoration)
{
    xdg_decorations *d = &xv->decorations;
    detach_xdg(d);
    d->xdg = decoration;
    listener_connect(&d->xdg_request_mode, &decoration->events.request_mode,
                     handle_xdg_request_mode);
    listener_connect(&d->xdg_destroy, &decoration->events.destroy, handle_xdg_destroy);
    refresh_mode(xv);
}

void xdg_view_attach_kde_decoration(xdg_view *xv, struct wlr_server_decoration *decoration)
{
    xdg_decorations *d = &xv->decorations;
    detach_kde(d);
    d->kde = decoration;
    listener_connect(&d->kde_mode, &decoration->events.mode, handle_kde_mode);
    listener_connect(&d->kde_destroy, &decoration->events.destroy, handle_kde_destroy);
    refresh_mode(xv);
}

void xdg_decorations_send_mode(xdg_view *xv)
{
    xdg_decorations *d = &xv->decorations;
    if (!xv->toplevel->base->initialized) {
        return;
    }
    if (xv->view == NULL) {
        xdg_view_send_bounds(xv);
    }
    if (d->xdg == NULL) {
        return;
    }
    enum wlr_xdg_toplevel_decoration_v1_mode mode =
        d->mode == DECORATION_SERVER ? WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE
                                     : WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE;
    wlr_xdg_toplevel_decoration_v1_set_mode(d->xdg, mode);
}

static void handle_xdg_request_mode(struct wl_listener *listener, void *data)
{
    (void) data;
    xdg_view *xv = wl_container_of(listener, xv, decorations.xdg_request_mode);
    refresh_mode(xv);
}

static void handle_xdg_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    xdg_view *xv = wl_container_of(listener, xv, decorations.xdg_destroy);
    detach_xdg(&xv->decorations);
    refresh_mode(xv);
}

static void handle_kde_mode(struct wl_listener *listener, void *data)
{
    (void) data;
    xdg_view *xv = wl_container_of(listener, xv, decorations.kde_mode);
    refresh_mode(xv);
}

static void handle_kde_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    xdg_view *xv = wl_container_of(listener, xv, decorations.kde_destroy);
    detach_kde(&xv->decorations);
    refresh_mode(xv);
}

static void detach_xdg(xdg_decorations *d)
{
    listener_disconnect(&d->xdg_request_mode);
    listener_disconnect(&d->xdg_destroy);
    d->xdg = NULL;
}

static void detach_kde(xdg_decorations *d)
{
    listener_disconnect(&d->kde_mode);
    listener_disconnect(&d->kde_destroy);
    d->kde = NULL;
}

static void refresh_mode(xdg_view *xv)
{
    xdg_decorations *d = &xv->decorations;
    decoration_mode mode = wanted_mode(d);
    bool changed = mode != d->mode;
    d->mode = mode;
    xdg_decorations_send_mode(xv);
    if (!changed || xv->view == NULL || view_window(xv->view) == NULL) {
        return;
    }
    wm_window_set_capabilities(xv->server->desktop, view_window(xv->view),
                               xdg_view_capabilities(xv));
}

static decoration_mode wanted_mode(const xdg_decorations *d)
{
    if (d->xdg != NULL) {
        return d->xdg->requested_mode == WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE
                   ? DECORATION_CLIENT
                   : DECORATION_SERVER;
    }
    if (d->kde != NULL) {
        return d->kde->mode == WLR_SERVER_DECORATION_MANAGER_MODE_SERVER ? DECORATION_SERVER
                                                                          : DECORATION_CLIENT;
    }
    return DECORATION_CLIENT;
}
