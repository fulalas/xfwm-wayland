#ifndef COMPOSITOR_XCURSOR_CACHE_H
#define COMPOSITOR_XCURSOR_CACHE_H

#include <stdbool.h>
#include <stdint.h>

struct wlr_buffer;
struct wlr_xcursor;
struct wlr_xcursor_image;

typedef struct xcursor_cache xcursor_cache;

typedef struct xcursor_fit {
    struct wlr_xcursor *xcursor;
    float load_scale;
} xcursor_fit;

xcursor_cache *xcursor_cache_create(const char *theme, uint32_t size);
void xcursor_cache_destroy(xcursor_cache *cache);
bool xcursor_cache_has_theme(const xcursor_cache *cache);
struct wlr_xcursor *xcursor_cache_get(xcursor_cache *cache, const char *name, float scale);
xcursor_fit xcursor_cache_fit(xcursor_cache *cache, const char *name, float scale,
                              float oversample);
struct wlr_buffer *xcursor_cache_image_buffer(struct wlr_xcursor_image *image);

#endif
