#ifndef XFCE_ICON_CACHE_H
#define XFCE_ICON_CACHE_H

#include <cairo.h>

typedef struct icon_cache icon_cache;

icon_cache *icon_cache_create(void);
void icon_cache_destroy(icon_cache *cache);
cairo_surface_t *icon_cache_load(icon_cache *cache, const char *path, int size);

#endif
