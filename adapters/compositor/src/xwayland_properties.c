#include <stdlib.h>
#include <wlr/xwayland/xwayland.h>

#include "listener.h"
#include "output.h"
#include "xwayland_shell_private.h"
#include "xwayland_view_private.h"

static void handle_title(struct wl_listener *listener, void *data);
static void handle_class(struct wl_listener *listener, void *data);
static void handle_parent(struct wl_listener *listener, void *data);
static void handle_window_type(struct wl_listener *listener, void *data);
static void handle_hints(struct wl_listener *listener, void *data);
static void handle_size_hints(struct wl_listener *listener, void *data);
static void handle_decorations(struct wl_listener *listener, void *data);
static void handle_strut_partial(struct wl_listener *listener, void *data);
static void handle_geometry(struct wl_listener *listener, void *data);
static void handle_opacity(struct wl_listener *listener, void *data);
static void handle_icon(struct wl_listener *listener, void *data);
static void handle_ping_timeout(struct wl_listener *listener, void *data);
static void handle_startup_id(struct wl_listener *listener, void *data);

void xwayland_properties_connect(xwayland_view *xv)
{
    struct wlr_xwayland_surface *xs = xv->xsurface;
    xwayland_properties *p = &xv->properties;
    listener_connect(&p->title, &xs->events.set_title, handle_title);
    listener_connect(&p->class, &xs->events.set_class, handle_class);
    listener_connect(&p->parent, &xs->events.set_parent, handle_parent);
    listener_connect(&p->window_type, &xs->events.set_window_type, handle_window_type);
    listener_connect(&p->hints, &xs->events.set_hints, handle_hints);
    listener_connect(&p->size_hints, &xs->events.set_size_hints, handle_size_hints);
    listener_connect(&p->decorations, &xs->events.set_decorations, handle_decorations);
    listener_connect(&p->strut_partial, &xs->events.set_strut_partial, handle_strut_partial);
    listener_connect(&p->geometry, &xs->events.set_geometry, handle_geometry);
    listener_connect(&p->opacity, &xs->events.set_opacity, handle_opacity);
    listener_connect(&p->icon, &xs->events.set_icon, handle_icon);
    listener_connect(&p->ping_timeout, &xs->events.ping_timeout, handle_ping_timeout);
    listener_connect(&p->startup_id, &xs->events.set_startup_id, handle_startup_id);
}

void xwayland_properties_disconnect(xwayland_view *xv)
{
    xwayland_properties *p = &xv->properties;
    struct wl_listener *all[] = {
        &p->title,       &p->class,       &p->parent,        &p->window_type,
        &p->hints,       &p->size_hints,  &p->decorations,   &p->strut_partial,
        &p->geometry,    &p->opacity,     &p->icon,          &p->ping_timeout,
        &p->startup_id,
    };
    for (size_t i = 0; i < sizeof(all) / sizeof(all[0]); i++) {
        listener_disconnect(all[i]);
    }
}

static void handle_title(struct wl_listener *listener, void *data)
{
    (void) data;
    xwayland_view *xv = wl_container_of(listener, xv, properties.title);
    wm_window *window = xwayland_view_window(xv);
    if (window == NULL) {
        return;
    }
    char *remote_title = xwayland_view_remote_title(xv);
    wm_window_set_title(xv->server->desktop, window,
                        remote_title != NULL ? remote_title : xv->xsurface->title);
    free(remote_title);
}

static void handle_class(struct wl_listener *listener, void *data)
{
    (void) data;
    xwayland_view *xv = wl_container_of(listener, xv, properties.class);
    wm_window *window = xwayland_view_window(xv);
    if (window != NULL) {
        wm_window_set_app_id(xv->server->desktop, window, xv->xsurface->class);
    }
    icon_slot_lookup_app(xv->icon, xv->xsurface->class);
}

static void handle_parent(struct wl_listener *listener, void *data)
{
    (void) data;
    xwayland_view *xv = wl_container_of(listener, xv, properties.parent);
    wm_window *window = xwayland_view_window(xv);
    if (window != NULL) {
        wm_window_set_parent(xv->server->desktop, window, xwayland_view_parent_window(xv));
    }
}

static void handle_window_type(struct wl_listener *listener, void *data)
{
    (void) data;
    xwayland_view *xv = wl_container_of(listener, xv, properties.window_type);
    wm_window *window = xwayland_view_window(xv);
    if (window != NULL) {
        wm_window_set_type(xv->server->desktop, window, xwayland_view_type(xv));
    }
}

static void handle_hints(struct wl_listener *listener, void *data)
{
    (void) data;
    xwayland_view *xv = wl_container_of(listener, xv, properties.hints);
    xwayland_view_report_urgency(xv);
}

static void handle_startup_id(struct wl_listener *listener, void *data)
{
    (void) data;
    xwayland_view *xv = wl_container_of(listener, xv, properties.startup_id);
    wm_window *window = xwayland_view_window(xv);
    if (xv->server->xwayland != NULL) {
        xwayland_shell_note_startup_id(xv->server->xwayland, xv->xsurface->pid,
                                       xv->xsurface->startup_id);
    }
    if (window != NULL) {
        wm_window_launched(xv->server->desktop, window, xv->xsurface->startup_id);
    }
}

static void handle_size_hints(struct wl_listener *listener, void *data)
{
    (void) data;
    xwayland_view *xv = wl_container_of(listener, xv, properties.size_hints);
    wm_window *window = xwayland_view_window(xv);
    if (window != NULL) {
        wm_size_hints hints = xwayland_view_size_hints(xv);
        wm_window_set_size_hints(xv->server->desktop, window, &hints);
    }
}

static void handle_decorations(struct wl_listener *listener, void *data)
{
    (void) data;
    xwayland_view *xv = wl_container_of(listener, xv, properties.decorations);
    xwayland_view_read_motif(xv);
    wm_window *window = xwayland_view_window(xv);
    if (window != NULL) {
        wm_window_set_capabilities(xv->server->desktop, window, xwayland_view_capabilities(xv));
    }
}

static void handle_strut_partial(struct wl_listener *listener, void *data)
{
    (void) data;
    xwayland_view *xv = wl_container_of(listener, xv, properties.strut_partial);
    if (xv->view != NULL) {
        outputs_schedule_arrange(xv->server->outputs);
    }
}

static void handle_geometry(struct wl_listener *listener, void *data)
{
    (void) data;
    xwayland_view *xv = wl_container_of(listener, xv, properties.geometry);
    if (xv->unmanaged != NULL) {
        unmanaged_move(xv->unmanaged);
    }
    if (xv->view != NULL) {
        xwayland_view_settle_if_current(xv);
    }
}

static void handle_opacity(struct wl_listener *listener, void *data)
{
    (void) data;
    xwayland_view *xv = wl_container_of(listener, xv, properties.opacity);
    wm_window *window = xwayland_view_window(xv);
    if (window != NULL) {
        wm_window_set_opacity(xv->server->desktop, window, xv->xsurface->opacity);
    }
}

static void handle_icon(struct wl_listener *listener, void *data)
{
    (void) data;
    xwayland_view *xv = wl_container_of(listener, xv, properties.icon);
    if (xv->view != NULL) {
        xwayland_view_fetch_icon(xv);
    }
}

static void handle_ping_timeout(struct wl_listener *listener, void *data)
{
    (void) data;
    xwayland_view *xv = wl_container_of(listener, xv, properties.ping_timeout);
    if (xv->view != NULL) {
        view_report_not_responding(xv->view);
    }
}
