#ifndef COMPOSITOR_XFCE_TOPLEVEL_PRIVATE_H
#define COMPOSITOR_XFCE_TOPLEVEL_PRIVATE_H

#include <cairo.h>
#include <stdbool.h>
#include <stdint.h>

#include "xfce-foreign-toplevel-management-private-v1-protocol.h"
#include "xfce_toplevel.h"

typedef struct xfce_published {
    uint32_t states;
    unsigned icon_serial;
    bool icon_sent;
    struct wl_resource *workspace;
} xfce_published;

typedef struct xfce_handle {
    xfce_toplevels *toplevels;
    struct wl_resource *resource;
    struct wl_resource *wlr_resource;
    toplevel_entry *entry;
    xfce_published published;
    struct wl_listener wlr_destroy;
    struct wl_listener workspace_destroy;
    struct wl_list link;
} xfce_handle;

struct xfce_toplevels {
    compositor *server;
    struct wl_global *global;
    struct wl_client *highlight_owner;
    struct wl_listener owner_destroy;
};

void xfce_handle_publish(xfce_handle *handle);
void xfce_handle_forget_workspace(xfce_handle *handle);
const struct xfce_foreign_toplevel_handle_v1_interface *xfce_handle_requests(void);
void xfce_handle_highlight(struct wl_client *client, struct wl_resource *resource,
                           struct wl_resource *seat);
void xfce_handle_unhighlight(struct wl_client *client, struct wl_resource *resource);
void xfce_toplevels_release_highlight(xfce_toplevels *toplevels);
typedef struct icon_size {
    uint32_t size;
    uint32_t scale;
} icon_size;

icon_size xfce_icon_size_of(cairo_surface_t *pixels);
struct wl_resource *xfce_icon_pixels_create(struct wl_resource *parent, uint32_t id);
void xfce_icon_pixels_answer(struct wl_resource *answer, cairo_surface_t *pixels, icon_size wanted);

#endif
