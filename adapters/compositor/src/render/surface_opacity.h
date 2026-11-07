#ifndef COMPOSITOR_SURFACE_OPACITY_H
#define COMPOSITOR_SURFACE_OPACITY_H

#include <stdbool.h>

struct wlr_surface;

bool surface_is_opaque(const struct wlr_surface *surface);

#endif
