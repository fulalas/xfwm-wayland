#ifndef COMPOSITOR_SHADOW_CACHE_H
#define COMPOSITOR_SHADOW_CACHE_H

#include "compositor/painter.h"
#include "server/server.h"
#include "shadow_layout.h"

struct wlr_buffer;

typedef struct shadow_image {
    struct wlr_buffer *buffer;
    shadow_geometry geometry;
} shadow_image;

shadow_cache *shadow_cache_create(const compositor_painter *painter);
void shadow_cache_destroy(shadow_cache *cache);
shadow_image shadow_cache_get(shadow_cache *cache);

#endif
