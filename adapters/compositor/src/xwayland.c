#include <stdio.h>
#include <stdlib.h>
#include <wlr/types/wlr_buffer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/util/log.h>
#include <wlr/xcursor.h>
#include <wlr/xwayland/server.h>
#include <wlr/xwayland/xwayland.h>

#include "cairo_buffer.h"
#include "keyboard_grabs.h"
#include "listener.h"
#include "seat.h"
#include "xwayland_ewmh.h"
#include "xwayland_shell_private.h"
#include "xwayland_view.h"
#include "xwayland_xsettings.h"

enum {
    BYTES_PER_PIXEL = 4,
};

static bool create_parts(xwayland_shell *shell);
static void handle_ready(struct wl_listener *listener, void *data);
static void handle_new_surface(struct wl_listener *listener, void *data);
static bool handle_x11_event(struct wlr_xwayland *wlr, xcb_generic_event_t *event);
static void set_default_cursor(xwayland_shell *shell);

xwayland_shell *xwayland_shell_create(compositor *server)
{
    xwayland_shell *shell = calloc(1, sizeof(*shell));
    if (shell == NULL) {
        return NULL;
    }
    shell->server = server;
    wl_list_init(&shell->views);
    wl_list_init(&shell->ready.link);
    wl_list_init(&shell->new_surface.link);
    xwayland_restart_init(shell);
    if (!create_parts(shell)) {
        xwayland_shell_destroy(shell);
        return NULL;
    }
    xwayland_shell_start(shell);
    return shell;
}

void xwayland_shell_destroy(xwayland_shell *shell)
{
    xwayland_restart_finish(shell);
    xwayland_shell_stop(shell);
    if (shell->grabs != NULL) {
        keyboard_grabs_destroy(shell->grabs);
    }
    if (shell->xsettings != NULL) {
        xwayland_xsettings_destroy(shell->xsettings);
    }
    if (shell->ewmh != NULL) {
        xwayland_ewmh_destroy(shell->ewmh);
    }
    free(shell);
}

bool xwayland_shell_start(xwayland_shell *shell)
{
    compositor *server = shell->server;
    shell->wlr = wlr_xwayland_create(server->display, server->globals.compositor, true);
    if (shell->wlr == NULL) {
        wlr_log(WLR_ERROR, "cannot start XWayland, X11 applications are unavailable");
        return false;
    }
    shell->wlr->data = shell;
    shell->wlr->user_event_handler = handle_x11_event;
    snprintf(shell->display_name, sizeof(shell->display_name), "%s", shell->wlr->display_name);
    listener_connect(&shell->ready, &shell->wlr->events.ready, handle_ready);
    listener_connect(&shell->new_surface, &shell->wlr->events.new_surface, handle_new_surface);
    xwayland_restart_watch(shell);
    return true;
}

void xwayland_shell_stop(xwayland_shell *shell)
{
    xwayland_restart_unwatch(shell);
    listener_disconnect(&shell->ready);
    listener_disconnect(&shell->new_surface);
    shell->link = (x11_link){.wlr = NULL, .ready = false};
    if (shell->wlr != NULL) {
        wlr_xwayland_destroy(shell->wlr);
        shell->wlr = NULL;
    }
}

const char *xwayland_shell_display_name(const xwayland_shell *shell)
{
    return shell != NULL && shell->wlr != NULL ? shell->display_name : NULL;
}

wm_rect xwayland_shell_apply_struts(const xwayland_shell *shell, const output *o, wm_rect usable)
{
    struct wlr_box root = {0};
    wlr_output_layout_get_box(shell->server->layout, NULL, &root);
    strut_context context = {
        .output = output_full(o),
        .root = {root.x, root.y, root.width, root.height},
    };
    return xwayland_views_apply_struts(&shell->views, &context, usable);
}

void xwayland_shell_publish_workspaces(xwayland_shell *shell, const wm_workspaces_view *view)
{
    xwayland_ewmh_publish_workspaces(shell->ewmh, view);
}

void xwayland_shell_outputs_arranged(xwayland_shell *shell)
{
    xwayland_ewmh_publish_outputs(shell->ewmh);
}

const x11_link *xwayland_shell_link(const xwayland_shell *shell)
{
    return &shell->link;
}

struct wl_client *xwayland_shell_client(const xwayland_shell *shell)
{
    if (shell->wlr == NULL || shell->wlr->server == NULL) {
        return NULL;
    }
    return shell->wlr->server->client;
}

bool xwayland_shell_grabs_keyboard(const xwayland_shell *shell, struct wlr_surface *focused)
{
    return shell != NULL && keyboard_grabs_hold(shell->grabs, focused);
}

void compositor_x11_settings_changed(compositor *c)
{
    if (c->xwayland != NULL) {
        xwayland_xsettings_reload(c->xwayland->xsettings);
    }
}

static bool create_parts(xwayland_shell *shell)
{
    shell->ewmh = xwayland_ewmh_create(shell->server, &shell->link);
    shell->xsettings = xwayland_xsettings_create(shell->server, &shell->link);
    shell->grabs = keyboard_grabs_create(shell->server->display, shell);
    return shell->ewmh != NULL && shell->xsettings != NULL && shell->grabs != NULL &&
           shell->restart.timer != NULL;
}

static void handle_ready(struct wl_listener *listener, void *data)
{
    (void) data;
    xwayland_shell *shell = wl_container_of(listener, shell, ready);
    wlr_xwayland_set_seat(shell->wlr, seat_wlr(shell->server->seat));
    set_default_cursor(shell);
    x11_link_attach(&shell->link, shell->wlr);
    xwayland_ewmh_link_ready(shell->ewmh);
    xwayland_xsettings_link_ready(shell->xsettings);
}

static void handle_new_surface(struct wl_listener *listener, void *data)
{
    xwayland_shell *shell = wl_container_of(listener, shell, new_surface);
    if (xwayland_view_create(shell->server, data, &shell->views) == NULL) {
        wlr_log(WLR_ERROR, "cannot track a new X11 window");
    }
}

static bool handle_x11_event(struct wlr_xwayland *wlr, xcb_generic_event_t *event)
{
    xwayland_shell *shell = wlr->data;
    if (xwayland_xsettings_handle_event(shell->xsettings, event)) {
        return true;
    }
    return xwayland_messages_handle(shell, event);
}

static void set_default_cursor(xwayland_shell *shell)
{
    struct wlr_xcursor_manager *xcursors = seat_xcursors(shell->server->seat);
    struct wlr_xcursor *xcursor =
        xcursors != NULL ? wlr_xcursor_manager_get_xcursor(xcursors, "default", 1.0f) : NULL;
    if (xcursor == NULL || xcursor->image_count == 0) {
        return;
    }
    struct wlr_xcursor_image *image = xcursor->images[0];
    pixel_rows rows = {image->buffer, (int) image->width, (int) image->height,
                       (int) image->width * BYTES_PER_PIXEL};
    cairo_surface_t *pixels = cairo_buffer_copy_pixels(&rows);
    struct wlr_buffer *buffer = pixels != NULL ? cairo_buffer_create(pixels) : NULL;
    if (buffer != NULL) {
        wlr_xwayland_set_cursor(shell->wlr, buffer, (int32_t) image->hotspot_x,
                                (int32_t) image->hotspot_y);
        wlr_buffer_drop(buffer);
    }
    if (pixels != NULL) {
        cairo_surface_destroy(pixels);
    }
}
