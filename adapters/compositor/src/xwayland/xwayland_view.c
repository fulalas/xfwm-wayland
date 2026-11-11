#include <stdlib.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/util/log.h>
#include <wlr/xwayland/xwayland.h>

#include "util/listener.h"
#include "output/output.h"
#include "x11_scale.h"
#include "xwayland_shell_private.h"
#include "xwayland_view_private.h"

static void handle_destroy(struct wl_listener *listener, void *data);
static void handle_associate(struct wl_listener *listener, void *data);
static void handle_dissociate(struct wl_listener *listener, void *data);
static void handle_map(struct wl_listener *listener, void *data);
static void handle_unmap(struct wl_listener *listener, void *data);
static void handle_commit(struct wl_listener *listener, void *data);
static void handle_icon_changed(void *self);
static void detach_surface(xwayland_view *view);
static void map_managed(xwayland_view *view);
static void unmap_any(xwayland_view *view);
static void release_queries_when_idle(const xwayland_view *view);
static struct wlr_surface *impl_surface(void *self);
static void impl_configure(void *self, const wm_rect *client);
static void impl_close(void *self);
static pid_t impl_pid(void *self);
static void impl_focus(void *self);
static void impl_unfocus(void *self);
static icon_slot *impl_icon(void *self);

static const view_impl xwayland_view_impl = {
    .surface = impl_surface,
    .configure = impl_configure,
    .close = impl_close,
    .pid = impl_pid,
    .send_states = xwayland_view_send_states,
    .focus = impl_focus,
    .unfocus = impl_unfocus,
    .icon = impl_icon,
};

xwayland_view *xwayland_view_create(compositor *server, struct wlr_xwayland_surface *xsurface,
                                    struct wl_list *views)
{
    xwayland_view *xv = calloc(1, sizeof(*xv));
    if (xv == NULL) {
        return NULL;
    }
    xv->server = server;
    xv->xsurface = xsurface;
    xv->motif_capabilities = WM_CAP_ALL;
    xv->icon = icon_slot_create(server, &(icon_listener){xv, handle_icon_changed});
    xsurface->data = xv;
    wl_list_init(&xv->map.link);
    wl_list_init(&xv->unmap.link);
    wl_list_init(&xv->commit.link);
    listener_connect(&xv->destroy, &xsurface->events.destroy, handle_destroy);
    listener_connect(&xv->associate, &xsurface->events.associate, handle_associate);
    listener_connect(&xv->dissociate, &xsurface->events.dissociate, handle_dissociate);
    xwayland_requests_connect(xv);
    xwayland_properties_connect(xv);
    wl_list_insert(views, &xv->link);
    return xv;
}

void xwayland_views_rescale(struct wl_list *views)
{
    xwayland_view *xv;
    wl_list_for_each(xv, views, link) {
        if (xv->unmanaged != NULL) {
            unmanaged_move(xv->unmanaged);
        }
        wm_window *window = xwayland_view_window(xv);
        if (window != NULL) {
            wm_rect client = wm_window_current_view(window).client;
            impl_configure(xv, &client);
        }
    }
}

wm_rect xwayland_views_apply_struts(const struct wl_list *views, const strut_context *context,
                                    wm_rect usable)
{
    xwayland_view *xv;
    wl_list_for_each(xv, views, link) {
        strut s = xwayland_view_strut(xv);
        usable = usable_area_apply_strut(usable, context, &s);
    }
    return usable;
}

wm_window *xwayland_view_window(const xwayland_view *xv)
{
    return xv->view != NULL ? view_window(xv->view) : NULL;
}

double xwayland_view_scale(const xwayland_view *xv)
{
    return xwayland_shell_scale(xv->server->xwayland);
}

void xwayland_view_settle_if_current(xwayland_view *xv)
{
    const struct wlr_surface_state *current = &xv->xsurface->surface->current;
    wm_size committed = {current->width, current->height};
    wm_size server_size = {xv->xsurface->width, xv->xsurface->height};
    if (wm_size_equals(committed, server_size)) {
        view_configure_settled(xv->view);
    }
}

static void handle_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    xwayland_view *xv = wl_container_of(listener, xv, destroy);
    unmap_any(xv);
    detach_surface(xv);
    xwayland_requests_disconnect(xv);
    xwayland_properties_disconnect(xv);
    listener_disconnect(&xv->destroy);
    listener_disconnect(&xv->associate);
    listener_disconnect(&xv->dissociate);
    wl_list_remove(&xv->link);
    xv->xsurface->data = NULL;
    icon_slot_destroy(xv->icon);
    free(xv);
}

static void handle_associate(struct wl_listener *listener, void *data)
{
    (void) data;
    xwayland_view *xv = wl_container_of(listener, xv, associate);
    struct wlr_surface *surface = xv->xsurface->surface;
    listener_connect(&xv->map, &surface->events.map, handle_map);
    listener_connect(&xv->unmap, &surface->events.unmap, handle_unmap);
    listener_connect(&xv->commit, &surface->events.commit, handle_commit);
}

static void handle_dissociate(struct wl_listener *listener, void *data)
{
    (void) data;
    xwayland_view *xv = wl_container_of(listener, xv, dissociate);
    unmap_any(xv);
    detach_surface(xv);
}

static void handle_map(struct wl_listener *listener, void *data)
{
    (void) data;
    xwayland_view *xv = wl_container_of(listener, xv, map);
    if (xv->xsurface->override_redirect) {
        xv->unmanaged = unmanaged_create(xv->server, xv->xsurface);
        return;
    }
    map_managed(xv);
}

static void handle_unmap(struct wl_listener *listener, void *data)
{
    (void) data;
    xwayland_view *xv = wl_container_of(listener, xv, unmap);
    unmap_any(xv);
}

static void handle_commit(struct wl_listener *listener, void *data)
{
    (void) data;
    xwayland_view *xv = wl_container_of(listener, xv, commit);
    if (xv->unmanaged != NULL) {
        unmanaged_committed(xv->unmanaged);
    }
    if (xv->view != NULL) {
        struct wlr_surface *surface = xv->xsurface->surface;
        wm_size size = {surface->current.width, surface->current.height};
        view_committed(xv->view, x11_scale_size_from_x(xwayland_view_scale(xv), size));
        xwayland_view_settle_if_current(xv);
    }
}

static void handle_icon_changed(void *self)
{
    xwayland_view *xv = self;
    if (xv->view != NULL) {
        view_icon_changed(xv->view);
    }
}

static void detach_surface(xwayland_view *xv)
{
    listener_disconnect(&xv->map);
    listener_disconnect(&xv->unmap);
    listener_disconnect(&xv->commit);
}

static void map_managed(xwayland_view *xv)
{
    xv->view = view_create(xv->server, &xwayland_view_impl, xv);
    if (xv->view == NULL) {
        wlr_log(WLR_ERROR, "cannot create a view for X11 window %s", xv->xsurface->class);
        return;
    }
    xwayland_surface_tree(xv->server, view_content_tree(xv->view), xv->xsurface->surface);
    xwayland_view_read_properties(xv);
    wm_window_desc desc = xwayland_view_describe(xv);
    char *remote_title = xwayland_view_remote_title(xv);
    if (remote_title != NULL) {
        desc.title = remote_title;
    }
    view_map(xv->view, &desc);
    free(remote_title);
    xwayland_view_fetch_icon(xv);
    if (xwayland_view_has_strut(xv)) {
        outputs_schedule_arrange(xv->server->outputs);
    }
}

static void unmap_any(xwayland_view *xv)
{
    if (xv->unmanaged != NULL) {
        unmanaged_destroy(xv->unmanaged);
        xv->unmanaged = NULL;
    }
    if (xv->view == NULL) {
        return;
    }
    view *v = xv->view;
    xv->view = NULL;
    view_destroy(v);
    release_queries_when_idle(xv);
    if (xwayland_view_has_strut(xv)) {
        outputs_schedule_arrange(xv->server->outputs);
    }
}

static void release_queries_when_idle(const xwayland_view *xv)
{
    xwayland_shell *shell = xv->server->xwayland;
    if (shell == NULL) {
        return;
    }
    const xwayland_view *other;
    wl_list_for_each(other, &shell->views, link) {
        if (other->view != NULL) {
            return;
        }
    }
    x11_link_release_queries(&shell->link);
}

static struct wlr_surface *impl_surface(void *self)
{
    xwayland_view *xv = self;
    return xv->xsurface->surface;
}

static void impl_configure(void *self, const wm_rect *client)
{
    xwayland_view *xv = self;
    wm_rect x = x11_scale_rect_to_x(xwayland_view_scale(xv), *client);
    wlr_xwayland_surface_configure(xv->xsurface, (int16_t) x.x, (int16_t) x.y, (uint16_t) x.width,
                                   (uint16_t) x.height);
}

static void impl_close(void *self)
{
    xwayland_view *xv = self;
    wlr_xwayland_surface_close(xv->xsurface);
    wlr_xwayland_surface_ping(xv->xsurface);
}

static pid_t impl_pid(void *self)
{
    xwayland_view *xv = self;
    return xv->xsurface->pid;
}

static void impl_focus(void *self)
{
    xwayland_view *xv = self;
    if (wlr_xwayland_surface_icccm_input_model(xv->xsurface) == WLR_ICCCM_INPUT_MODEL_GLOBAL) {
        wlr_xwayland_surface_offer_focus(xv->xsurface);
        return;
    }
    wlr_xwayland_surface_activate(xv->xsurface, true);
}

static void impl_unfocus(void *self)
{
    xwayland_view *xv = self;
    wlr_xwayland_surface_activate(xv->xsurface, false);
}

static icon_slot *impl_icon(void *self)
{
    xwayland_view *xv = self;
    return xv->icon;
}
