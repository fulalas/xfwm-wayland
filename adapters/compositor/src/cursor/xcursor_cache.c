#include "xcursor_cache.h"

#include <glib.h>
#include <math.h>
#include <stdbool.h>
#include <wayland-util.h>
#include <wlr/types/wlr_buffer.h>
#include <wlr/xcursor.h>

#include "render/cairo_buffer.h"
#include "xcursor_file.h"

typedef struct cached_image {
    struct wlr_xcursor_image image;
    struct wlr_buffer *buffer;
} cached_image;

enum {
    FALLBACK_SLOTS = 4,
};

typedef struct fallback_slot {
    uint32_t size;
    uint64_t used;
    bool loaded;
    struct wlr_xcursor_theme *theme;
} fallback_slot;

typedef struct cached_cursor {
    struct wlr_xcursor xcursor;
    bool larger;
} cached_cursor;

struct xcursor_cache {
    char *theme;
    uint32_t size;
    char **dirs;
    bool found_any;
    uint64_t fallback_clock;
    fallback_slot fallback[FALLBACK_SLOTS];
    GHashTable *cursors;
};

static cached_cursor *lookup(xcursor_cache *cache, const char *name,
                             const xcursor_request *request);
static cached_cursor *load_cursor(xcursor_cache *cache, const char *name,
                                  const xcursor_request *request);
static void fill_from_frames(cached_cursor *cursor, const xcursor_frames *frames);
static void fill_from_fallback(xcursor_cache *cache, cached_cursor *cursor, const char *name,
                               const xcursor_request *request);
static struct wlr_xcursor *fallback_cursor(xcursor_cache *cache, const char *name, int size);
static struct wlr_xcursor_theme *fallback_theme(xcursor_cache *cache, uint32_t size);
static fallback_slot *fallback_slot_for(xcursor_cache *cache, uint32_t size);
static bool add_image(cached_cursor *cursor, const struct wlr_xcursor_image *source);
static void drop_slot(fallback_slot *slot);
static void drop_fallback(xcursor_cache *cache);
static void destroy_cursor(gpointer data);

xcursor_cache *xcursor_cache_create(const char *theme, uint32_t size)
{
    xcursor_cache *cache = g_new0(xcursor_cache, 1);
    cache->theme = g_strdup(theme);
    cache->size = size;
    cache->dirs = xcursor_theme_dirs(theme);
    cache->cursors = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, destroy_cursor);
    return cache;
}

void xcursor_cache_destroy(xcursor_cache *cache)
{
    if (cache == NULL) {
        return;
    }
    g_hash_table_destroy(cache->cursors);
    drop_fallback(cache);
    g_strfreev(cache->dirs);
    g_free(cache->theme);
    g_free(cache);
}

bool xcursor_cache_has_theme(const xcursor_cache *cache)
{
    return cache->dirs[0] != NULL;
}

struct wlr_xcursor *xcursor_cache_get(xcursor_cache *cache, const char *name, float scale)
{
    xcursor_request request = {(int) ((float) cache->size * scale), 0, 0};
    cached_cursor *cursor = lookup(cache, name, &request);
    return cursor->xcursor.image_count > 0 ? &cursor->xcursor : NULL;
}

xcursor_fit xcursor_cache_fit(xcursor_cache *cache, const char *name, float scale,
                              float oversample)
{
    float load_scale = scale * oversample;
    xcursor_request request = {
        (int) ((float) cache->size * scale),
        (int) lroundf((float) cache->size * scale),
        (int) ((float) cache->size * load_scale),
    };
    cached_cursor *cursor = lookup(cache, name, &request);
    if (cursor->xcursor.image_count == 0) {
        return (xcursor_fit) {NULL, scale};
    }
    return (xcursor_fit) {&cursor->xcursor, cursor->larger ? load_scale : scale};
}

struct wlr_buffer *xcursor_cache_image_buffer(struct wlr_xcursor_image *image)
{
    cached_image *cached = wl_container_of(image, cached, image);
    return cached->buffer;
}

static cached_cursor *lookup(xcursor_cache *cache, const char *name,
                             const xcursor_request *request)
{
    char *key = g_strdup_printf("%d %d %d %s", request->size, request->width, request->larger,
                                name);
    cached_cursor *cursor = g_hash_table_lookup(cache->cursors, key);
    if (cursor != NULL) {
        g_free(key);
        return cursor;
    }
    cursor = load_cursor(cache, name, request);
    g_hash_table_insert(cache->cursors, key, cursor);
    return cursor;
}

static cached_cursor *load_cursor(xcursor_cache *cache, const char *name,
                                  const xcursor_request *request)
{
    cached_cursor *cursor = g_new0(cached_cursor, 1);
    cursor->xcursor.name = g_strdup(name);
    xcursor_frames frames;
    if (xcursor_theme_load(cache->dirs, name, request, &frames)) {
        cache->found_any = true;
        drop_fallback(cache);
        fill_from_frames(cursor, &frames);
        xcursor_frames_clear(&frames);
    } else if (!cache->found_any) {
        fill_from_fallback(cache, cursor, name, request);
    }
    return cursor;
}

static void fill_from_frames(cached_cursor *cursor, const xcursor_frames *frames)
{
    cursor->larger = frames->larger;
    cursor->xcursor.images = g_new0(struct wlr_xcursor_image *, frames->count);
    for (size_t i = 0; i < frames->count; i++) {
        const xcursor_frame *frame = &frames->frames[i];
        struct wlr_xcursor_image image = {
            .width = frame->width,
            .height = frame->height,
            .hotspot_x = frame->hotspot_x,
            .hotspot_y = frame->hotspot_y,
            .delay = frame->delay,
            .buffer = (uint8_t *) frame->pixels,
        };
        if (!add_image(cursor, &image)) {
            break;
        }
    }
}

static void fill_from_fallback(xcursor_cache *cache, cached_cursor *cursor, const char *name,
                               const xcursor_request *request)
{
    struct wlr_xcursor *xcursor = fallback_cursor(cache, name, request->size);
    if (xcursor == NULL) {
        return;
    }
    if (request->larger > 0 && xcursor->images[0]->width != (uint32_t) request->width) {
        struct wlr_xcursor *larger = fallback_cursor(cache, name, request->larger);
        if (larger != NULL) {
            xcursor = larger;
            cursor->larger = true;
        }
    }
    cursor->xcursor.images = g_new0(struct wlr_xcursor_image *, xcursor->image_count);
    for (unsigned int i = 0; i < xcursor->image_count; i++) {
        if (!add_image(cursor, xcursor->images[i])) {
            break;
        }
    }
}

static struct wlr_xcursor *fallback_cursor(xcursor_cache *cache, const char *name, int size)
{
    struct wlr_xcursor_theme *theme = size > 0 ? fallback_theme(cache, (uint32_t) size) : NULL;
    struct wlr_xcursor *xcursor = theme != NULL ? wlr_xcursor_theme_get_cursor(theme, name) : NULL;
    return xcursor != NULL && xcursor->image_count > 0 ? xcursor : NULL;
}

static struct wlr_xcursor_theme *fallback_theme(xcursor_cache *cache, uint32_t size)
{
    fallback_slot *slot = fallback_slot_for(cache, size);
    if (!slot->loaded) {
        slot->theme = wlr_xcursor_theme_load(cache->theme, size);
        slot->size = size;
        slot->loaded = true;
    }
    slot->used = ++cache->fallback_clock;
    return slot->theme;
}

static fallback_slot *fallback_slot_for(xcursor_cache *cache, uint32_t size)
{
    fallback_slot *oldest = &cache->fallback[0];
    for (int i = 0; i < FALLBACK_SLOTS; i++) {
        fallback_slot *slot = &cache->fallback[i];
        if (slot->loaded && slot->size == size) {
            return slot;
        }
        if (oldest->loaded && (!slot->loaded || slot->used < oldest->used)) {
            oldest = slot;
        }
    }
    drop_slot(oldest);
    return oldest;
}

static bool add_image(cached_cursor *cursor, const struct wlr_xcursor_image *source)
{
    pixel_rows rows = {source->buffer, (int) source->width, (int) source->height,
                       (int) source->width * (int) sizeof(uint32_t)};
    cairo_surface_t *surface = cairo_buffer_copy_pixels(&rows);
    struct wlr_buffer *buffer = surface != NULL ? cairo_buffer_create(surface) : NULL;
    if (buffer == NULL) {
        if (surface != NULL) {
            cairo_surface_destroy(surface);
        }
        return false;
    }
    cached_image *image = g_new0(cached_image, 1);
    image->image = *source;
    image->image.buffer = cairo_image_surface_get_data(surface);
    image->buffer = buffer;
    cairo_surface_destroy(surface);
    cursor->xcursor.images[cursor->xcursor.image_count++] = &image->image;
    cursor->xcursor.total_delay += source->delay;
    return true;
}

static void drop_slot(fallback_slot *slot)
{
    if (slot->theme != NULL) {
        wlr_xcursor_theme_destroy(slot->theme);
    }
    *slot = (fallback_slot) {0};
}

static void drop_fallback(xcursor_cache *cache)
{
    for (int i = 0; i < FALLBACK_SLOTS; i++) {
        drop_slot(&cache->fallback[i]);
    }
}

static void destroy_cursor(gpointer data)
{
    cached_cursor *cursor = data;
    for (unsigned int i = 0; i < cursor->xcursor.image_count; i++) {
        cached_image *image = wl_container_of(cursor->xcursor.images[i], image, image);
        wlr_buffer_drop(image->buffer);
        g_free(image);
    }
    g_free(cursor->xcursor.images);
    g_free(cursor->xcursor.name);
    g_free(cursor);
}
