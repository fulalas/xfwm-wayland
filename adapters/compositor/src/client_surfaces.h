#ifndef COMPOSITOR_CLIENT_SURFACES_H
#define COMPOSITOR_CLIENT_SURFACES_H

struct wl_display;
struct wlr_surface;

typedef void (*surface_visitor)(struct wlr_surface *surface, void *data);

void client_surfaces_for_each(struct wl_display *display, surface_visitor visit, void *data);

#endif
