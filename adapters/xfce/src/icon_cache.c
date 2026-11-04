#include "icon_cache.h"

#include <glib.h>
#include <string.h>

#include "icon_surface.h"

enum {
    ICON_CACHE_MAX = 32,
};

typedef struct icon_key {
    const char *path;
    int size;
} icon_key;

typedef struct cached_icon {
    char *path;
    int size;
    cairo_surface_t *surface;
} cached_icon;

struct icon_cache {
    GQueue entries;
};

static GList *find(const icon_cache *cache, const icon_key *key);
static cairo_surface_t *reuse(icon_cache *cache, GList *link);
static void remember(icon_cache *cache, const icon_key *key, cairo_surface_t *surface);
static void free_entry(void *data);

icon_cache *icon_cache_create(void)
{
    icon_cache *cache = g_new0(icon_cache, 1);

    g_queue_init(&cache->entries);
    return cache;
}

void icon_cache_destroy(icon_cache *cache)
{
    g_queue_clear_full(&cache->entries, free_entry);
    g_free(cache);
}

cairo_surface_t *icon_cache_load(icon_cache *cache, const char *path, int size)
{
    icon_key key = { path, size };
    GList *link = find(cache, &key);
    cairo_surface_t *surface;

    if (link) {
        return reuse(cache, link);
    }
    surface = icon_surface_load(path, size);
    if (surface) {
        remember(cache, &key, surface);
    }
    return surface;
}

static GList *find(const icon_cache *cache, const icon_key *key)
{
    for (GList *link = cache->entries.head; link; link = link->next) {
        const cached_icon *entry = link->data;
        if (entry->size == key->size && strcmp(entry->path, key->path) == 0) {
            return link;
        }
    }
    return NULL;
}

static cairo_surface_t *reuse(icon_cache *cache, GList *link)
{
    cached_icon *entry = link->data;

    g_queue_unlink(&cache->entries, link);
    g_queue_push_head_link(&cache->entries, link);
    return cairo_surface_reference(entry->surface);
}

static void remember(icon_cache *cache, const icon_key *key, cairo_surface_t *surface)
{
    cached_icon *entry = g_new(cached_icon, 1);

    *entry = (cached_icon) { g_strdup(key->path), key->size, cairo_surface_reference(surface) };
    g_queue_push_head(&cache->entries, entry);
    if (g_queue_get_length(&cache->entries) > ICON_CACHE_MAX) {
        free_entry(g_queue_pop_tail(&cache->entries));
    }
}

static void free_entry(void *data)
{
    cached_icon *entry = data;

    cairo_surface_destroy(entry->surface);
    g_free(entry->path);
    g_free(entry);
}
