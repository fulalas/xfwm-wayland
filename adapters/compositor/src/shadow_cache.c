#include <math.h>
#include <stdlib.h>
#include <wlr/types/wlr_buffer.h>

#include "cairo_buffer.h"
#include "shadow_cache.h"

enum {
    OPACITY_STEPS = 100,
};

struct shadow_cache {
    const compositor_painter *painter;
    shadow_image slots[OPACITY_STEPS + 1];
};

static int slot_for(double opacity);
static shadow_image load_image(const compositor_painter *painter, int slot);

shadow_cache *shadow_cache_create(const compositor_painter *painter)
{
    shadow_cache *cache = calloc(1, sizeof(*cache));
    if (cache != NULL) {
        cache->painter = painter;
    }
    return cache;
}

void shadow_cache_destroy(shadow_cache *cache)
{
    for (int i = 0; i <= OPACITY_STEPS; i++) {
        if (cache->slots[i].buffer != NULL) {
            wlr_buffer_drop(cache->slots[i].buffer);
        }
    }
    free(cache);
}

shadow_image shadow_cache_get(shadow_cache *cache, double opacity)
{
    int slot = slot_for(opacity);
    if (cache->slots[slot].buffer == NULL) {
        cache->slots[slot] = load_image(cache->painter, slot);
    }
    return cache->slots[slot];
}

static int slot_for(double opacity)
{
    long slot = lround(opacity * OPACITY_STEPS);
    if (slot < 0) {
        return 0;
    }
    return slot > OPACITY_STEPS ? OPACITY_STEPS : (int) slot;
}

static shadow_image load_image(const compositor_painter *painter, int slot)
{
    shadow_image image = {0};
    shadow_template tmpl = painter->shadow_template(painter->self, (double) slot / OPACITY_STEPS);
    image.buffer = cairo_buffer_create(tmpl.surface);
    if (tmpl.surface != NULL) {
        cairo_surface_destroy(tmpl.surface);
    }
    if (image.buffer != NULL) {
        image.geometry.size = (wm_size){image.buffer->width, image.buffer->height};
        image.geometry.corner = tmpl.corner_size;
    }
    return image;
}
