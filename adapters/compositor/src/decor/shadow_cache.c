#include <stdlib.h>
#include <wlr/types/wlr_buffer.h>

#include "render/cairo_buffer.h"
#include "shadow_cache.h"

struct shadow_cache {
    const compositor_painter *painter;
    shadow_image image;
};

static shadow_image load_image(const compositor_painter *painter);

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
    if (cache->image.buffer != NULL) {
        wlr_buffer_drop(cache->image.buffer);
    }
    free(cache);
}

shadow_image shadow_cache_get(shadow_cache *cache)
{
    if (cache->image.buffer == NULL) {
        cache->image = load_image(cache->painter);
    }
    return cache->image;
}

static shadow_image load_image(const compositor_painter *painter)
{
    shadow_image image = {0};
    shadow_template tmpl = painter->shadow_template(painter->self);
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
