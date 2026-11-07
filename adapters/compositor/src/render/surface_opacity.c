#include "surface_opacity.h"

#include <pixman.h>
#include <wlr/types/wlr_compositor.h>

bool surface_is_opaque(const struct wlr_surface *surface)
{
    if (surface == NULL || surface->current.width <= 0 || surface->current.height <= 0) {
        return false;
    }
    pixman_box32_t box = {0, 0, surface->current.width, surface->current.height};
    pixman_region32_t *region = (pixman_region32_t *) &surface->opaque_region;
    return pixman_region32_contains_rectangle(region, &box) == PIXMAN_REGION_IN;
}
