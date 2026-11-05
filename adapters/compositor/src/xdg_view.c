#include <stdlib.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>

#include "listener.h"
#include "output.h"
#include "seat.h"
#include "xdg_session.h"
#include "xdg_view_private.h"

static const uint32_t WM_CAPABILITIES =
    WLR_XDG_TOPLEVEL_WM_CAPABILITIES_WINDOW_MENU | WLR_XDG_TOPLEVEL_WM_CAPABILITIES_MAXIMIZE |
    WLR_XDG_TOPLEVEL_WM_CAPABILITIES_FULLSCREEN | WLR_XDG_TOPLEVEL_WM_CAPABILITIES_MINIMIZE;

static void handle_commit(struct wl_listener *listener, void *data);
static void handle_map(struct wl_listener *listener, void *data);
static void handle_unmap(struct wl_listener *listener, void *data);
static void handle_destroy(struct wl_listener *listener, void *data);
static void handle_icon_changed(void *self);
static void addon_destroy(struct wlr_addon *addon);
static void send_initial_configure(xdg_view *view);
static void acknowledge_configure(xdg_view *view);
static void sync_size_hints(xdg_view *view);
static void request_size(xdg_view *view, wm_size size);
static bool same_hints(const wm_size_hints *a, const wm_size_hints *b);
static wm_size committed_size(const xdg_view *view);
static void unmap_view(xdg_view *view);
static void stop_waiting(xdg_view *view);
static void adopt_waiting_children(xdg_view *view);
static wm_rect popup_constraint(void *root);
static struct wlr_surface *impl_surface(void *self);
static void impl_configure(void *self, const wm_rect *client);
static void impl_close(void *self);
static pid_t impl_pid(void *self);
static void impl_focus(void *self);
static icon_slot *impl_icon(void *self);

static const struct wlr_addon_interface xdg_view_addon = {
    .name = "xfwm-xdg-view",
    .destroy = addon_destroy,
};

static const view_impl xdg_view_impl = {
    .surface = impl_surface,
    .configure = impl_configure,
    .close = impl_close,
    .pid = impl_pid,
    .send_states = xdg_view_send_states,
    .focus = impl_focus,
    .unfocus = impl_focus,
    .icon = impl_icon,
};

xdg_view *xdg_view_create(compositor *server, struct wlr_xdg_toplevel *toplevel)
{
    xdg_view *xv = calloc(1, sizeof(*xv));
    if (xv == NULL) {
        return NULL;
    }
    xv->server = server;
    xv->toplevel = toplevel;
    xv->popup_parent = (popup_parent){NULL, xv, popup_constraint};
    wl_list_init(&xv->waiting_children);
    wl_list_init(&xv->waiting_link);
    xv->icon = icon_slot_create(server, &(icon_listener){xv, handle_icon_changed});
    struct wlr_surface *surface = toplevel->base->surface;
    wlr_addon_init(&xv->addon, &surface->addons, &xdg_view_addon, &xdg_view_addon);
    listener_connect(&xv->commit, &surface->events.commit, handle_commit);
    listener_connect(&xv->map, &surface->events.map, handle_map);
    listener_connect(&xv->unmap, &surface->events.unmap, handle_unmap);
    listener_connect(&xv->destroy, &toplevel->events.destroy, handle_destroy);
    output *o = outputs_at_point(server->outputs, seat_pointer_position(server->seat));
    if (o != NULL) {
        output_announce_scale(o, surface);
    }
    xdg_requests_connect(xv);
    xdg_decorations_init(xv);
    return xv;
}

void xdg_view_parent_requested(xdg_view *xv, xdg_view *parent)
{
    stop_waiting(xv);
    if (parent != NULL && !parent->toplevel->base->surface->mapped) {
        wl_list_insert(&parent->waiting_children, &xv->waiting_link);
    }
}

xdg_view *xdg_view_from_toplevel(struct wlr_xdg_toplevel *toplevel)
{
    if (toplevel == NULL) {
        return NULL;
    }
    struct wlr_addon *addon =
        wlr_addon_find(&toplevel->base->surface->addons, &xdg_view_addon, &xdg_view_addon);
    if (addon == NULL) {
        return NULL;
    }
    xdg_view *xv = wl_container_of(addon, xv, addon);
    return xv;
}

icon_slot *xdg_view_icon_slot(const xdg_view *xv)
{
    return xv->icon;
}

const char *xdg_view_app_id(const xdg_view *xv)
{
    return xv->toplevel->app_id;
}

static void handle_commit(struct wl_listener *listener, void *data)
{
    (void) data;
    xdg_view *xv = wl_container_of(listener, xv, commit);
    struct wlr_xdg_surface *base = xv->toplevel->base;
    if (base->initial_commit) {
        send_initial_configure(xv);
        return;
    }
    if (xv->view == NULL) {
        return;
    }
    bool answering = xv->pending_serial != 0;
    acknowledge_configure(xv);
    sync_size_hints(xv);
    view_committed(xv->view, committed_size(xv));
    if (answering && xv->pending_serial == 0) {
        view_configure_settled(xv->view);
    }
}

static void handle_map(struct wl_listener *listener, void *data)
{
    (void) data;
    xdg_view *xv = wl_container_of(listener, xv, map);
    xv->view = view_create(xv->server, &xdg_view_impl, xv);
    if (xv->view == NULL) {
        wlr_log(WLR_ERROR, "cannot create a view for %s", xv->toplevel->app_id);
        adopt_waiting_children(xv);
        return;
    }
    wlr_scene_xdg_surface_create(view_content_tree(xv->view), xv->toplevel->base);
    xv->popup_parent.tree = view_popup_tree(xv->view);
    xv->toplevel->base->data = &xv->popup_parent;
    wm_window_desc desc = xdg_view_describe(xv);
    xdg_sessions_apply_restore(xv->server->xdg_sessions, xv->toplevel, &desc);
    xv->size_hints = desc.size_hints;
    view_map(xv->view, &desc);
    icon_slot_lookup_app(xv->icon, xv->toplevel->app_id);
    adopt_waiting_children(xv);
}

static void handle_unmap(struct wl_listener *listener, void *data)
{
    (void) data;
    xdg_view *xv = wl_container_of(listener, xv, unmap);
    unmap_view(xv);
}

static void handle_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    xdg_view *xv = wl_container_of(listener, xv, destroy);
    unmap_view(xv);
    stop_waiting(xv);
    xdg_view *child;
    xdg_view *tmp;
    wl_list_for_each_safe(child, tmp, &xv->waiting_children, waiting_link) {
        stop_waiting(child);
    }
    xdg_decorations_finish(xv);
    xdg_requests_disconnect(xv);
    listener_disconnect(&xv->commit);
    listener_disconnect(&xv->map);
    listener_disconnect(&xv->unmap);
    listener_disconnect(&xv->destroy);
    wlr_addon_finish(&xv->addon);
    icon_slot_destroy(xv->icon);
    free(xv);
}

static void handle_icon_changed(void *self)
{
    xdg_view *xv = self;
    if (xv->view != NULL) {
        view_icon_changed(xv->view);
    }
}

static void addon_destroy(struct wlr_addon *addon)
{
    wlr_addon_finish(addon);
}

static void send_initial_configure(xdg_view *xv)
{
    wlr_xdg_toplevel_set_wm_capabilities(xv->toplevel, WM_CAPABILITIES);
    xdg_decorations_send_mode(xv);
    wm_size size = xdg_sessions_initial_size(xv->server->xdg_sessions, xv->toplevel);
    wlr_xdg_toplevel_set_size(xv->toplevel, size.width, size.height);
}

static void acknowledge_configure(xdg_view *xv)
{
    uint32_t acked = xv->toplevel->base->current.configure_serial;
    if (xv->pending_serial == 0 || (int32_t) (acked - xv->pending_serial) < 0) {
        return;
    }
    xv->pending_serial = 0;
    if (xv->has_deferred_size) {
        xv->has_deferred_size = false;
        request_size(xv, xv->deferred_size);
    }
}

static void sync_size_hints(xdg_view *xv)
{
    wm_size_hints hints = xdg_view_size_hints(xv);
    wm_window *window = view_window(xv->view);
    if (window == NULL || same_hints(&hints, &xv->size_hints)) {
        return;
    }
    xv->size_hints = hints;
    wm_window_set_size_hints(xv->server->desktop, window, &hints);
}

static void request_size(xdg_view *xv, wm_size size)
{
    const struct wlr_xdg_toplevel_configure *scheduled = &xv->toplevel->scheduled;
    bool scheduled_same = wm_size_equals(size, (wm_size){scheduled->width, scheduled->height});
    if (scheduled_same && wm_size_equals(size, committed_size(xv))) {
        return;
    }
    xv->pending_serial = wlr_xdg_toplevel_set_size(xv->toplevel, size.width, size.height);
}

static bool same_hints(const wm_size_hints *a, const wm_size_hints *b)
{
    return wm_size_equals(a->min, b->min) && wm_size_equals(a->max, b->max);
}

static wm_size committed_size(const xdg_view *xv)
{
    const struct wlr_box *geometry = &xv->toplevel->base->geometry;
    return (wm_size){geometry->width, geometry->height};
}

static void unmap_view(xdg_view *xv)
{
    if (xv->view == NULL) {
        return;
    }
    xv->toplevel->base->data = NULL;
    xv->popup_parent.tree = NULL;
    view *v = xv->view;
    xv->view = NULL;
    xv->pending_serial = 0;
    xv->has_deferred_size = false;
    view_destroy(v);
}

static void stop_waiting(xdg_view *xv)
{
    wl_list_remove(&xv->waiting_link);
    wl_list_init(&xv->waiting_link);
}

static void adopt_waiting_children(xdg_view *xv)
{
    xdg_view *child;
    xdg_view *tmp;
    wl_list_for_each_safe(child, tmp, &xv->waiting_children, waiting_link) {
        stop_waiting(child);
        wlr_xdg_toplevel_set_parent(child->toplevel, xv->toplevel);
    }
}

static wm_rect popup_constraint(void *root)
{
    xdg_view *xv = root;
    if (xv->view == NULL) {
        return (wm_rect){0, 0, 0, 0};
    }
    wm_rect client = view_client_rect(xv->view);
    wm_rect area = view_output_area(xv->view);
    const struct wlr_box *geometry = &xv->toplevel->base->geometry;
    return (wm_rect){area.x - client.x + geometry->x, area.y - client.y + geometry->y,
                     area.width, area.height};
}

static struct wlr_surface *impl_surface(void *self)
{
    xdg_view *xv = self;
    return xv->toplevel->base->surface;
}

static void impl_configure(void *self, const wm_rect *client)
{
    xdg_view *xv = self;
    if (!xv->toplevel->base->initialized) {
        return;
    }
    wm_size size = {client->width, client->height};
    if (xv->pending_serial != 0) {
        xv->deferred_size = size;
        xv->has_deferred_size = true;
        return;
    }
    request_size(xv, size);
}

static void impl_close(void *self)
{
    xdg_view *xv = self;
    wlr_xdg_toplevel_send_close(xv->toplevel);
    wlr_xdg_surface_ping(xv->toplevel->base);
}

static pid_t impl_pid(void *self)
{
    xdg_view *xv = self;
    pid_t pid = 0;
    wl_client_get_credentials(wl_resource_get_client(xv->toplevel->resource), &pid, NULL, NULL);
    return pid;
}

static void impl_focus(void *self)
{
    (void) self;
}

static icon_slot *impl_icon(void *self)
{
    xdg_view *xv = self;
    return xv->icon;
}
