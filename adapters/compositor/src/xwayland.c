#include <stdlib.h>
#include <wlr/types/wlr_buffer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/util/log.h>
#include <wlr/xcursor.h>
#include <wlr/xwayland/xwayland.h>

#include "cairo_buffer.h"
#include "listener.h"
#include "seat.h"
#include "xwayland.h"
#include "xwayland_view.h"

enum {
    BYTES_PER_PIXEL = 4,
};

struct xwayland_shell {
    compositor *server;
    struct wlr_xwayland *wlr;
    struct wl_list views;
    struct wl_listener ready;
    struct wl_listener new_surface;
};

static void handle_ready(struct wl_listener *listener, void *data);
static void handle_new_surface(struct wl_listener *listener, void *data);
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
    shell->wlr = wlr_xwayland_create(server->display, server->globals.compositor, true);
    if (shell->wlr == NULL) {
        wlr_log(WLR_ERROR, "cannot start XWayland, X11 applications are unavailable");
        return shell;
    }
    listener_connect(&shell->ready, &shell->wlr->events.ready, handle_ready);
    listener_connect(&shell->new_surface, &shell->wlr->events.new_surface, handle_new_surface);
    return shell;
}

void xwayland_shell_destroy(xwayland_shell *shell)
{
    listener_disconnect(&shell->ready);
    listener_disconnect(&shell->new_surface);
    if (shell->wlr != NULL) {
        wlr_xwayland_destroy(shell->wlr);
    }
    free(shell);
}

const char *xwayland_shell_display_name(const xwayland_shell *shell)
{
    return shell != NULL && shell->wlr != NULL ? shell->wlr->display_name : NULL;
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

static void handle_ready(struct wl_listener *listener, void *data)
{
    (void) data;
    xwayland_shell *shell = wl_container_of(listener, shell, ready);
    wlr_xwayland_set_seat(shell->wlr, seat_wlr(shell->server->seat));
    set_default_cursor(shell);
}

static void handle_new_surface(struct wl_listener *listener, void *data)
{
    xwayland_shell *shell = wl_container_of(listener, shell, new_surface);
    if (xwayland_view_create(shell->server, data, &shell->views) == NULL) {
        wlr_log(WLR_ERROR, "cannot track a new X11 window");
    }
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
