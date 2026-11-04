#include "icon_lookups.h"

#include <glib.h>

#include "icon_cache.h"

typedef struct icon_pending {
    unsigned id;
    int size;
    void (*done)(void *context, cairo_surface_t *icon);
    void *context;
} icon_pending;

struct icon_lookups {
    GQueue pending;
    unsigned next_id;
    icon_cache *cache;
};

static icon_pending *take_pending(icon_lookups *lookups, unsigned id);
static void complete(icon_lookups *lookups, icon_pending *pending, const char *path);

icon_lookups *icon_lookups_create(void)
{
    icon_lookups *lookups = g_new0(icon_lookups, 1);

    g_queue_init(&lookups->pending);
    lookups->cache = icon_cache_create();
    return lookups;
}

void icon_lookups_destroy(icon_lookups *lookups)
{
    g_queue_clear_full(&lookups->pending, g_free);
    icon_cache_destroy(lookups->cache);
    g_free(lookups);
}

unsigned icon_lookups_add(icon_lookups *lookups, const icon_request *request)
{
    icon_pending *pending = g_new0(icon_pending, 1);

    *pending = (icon_pending) {
        .id = ++lookups->next_id,
        .size = request->size,
        .done = request->done,
        .context = request->context,
    };
    g_queue_push_tail(&lookups->pending, pending);
    return pending->id;
}

void icon_lookups_cancel(icon_lookups *lookups, void *context)
{
    GList *link = lookups->pending.head;

    while (link) {
        GList *next = link->next;
        icon_pending *pending = link->data;
        if (pending->context == context) {
            g_free(pending);
            g_queue_delete_link(&lookups->pending, link);
        }
        link = next;
    }
}

void icon_lookups_finish(icon_lookups *lookups, unsigned id, const char *path)
{
    icon_pending *pending = take_pending(lookups, id);

    if (pending) {
        complete(lookups, pending, path);
    }
}

static icon_pending *take_pending(icon_lookups *lookups, unsigned id)
{
    for (GList *link = lookups->pending.head; link; link = link->next) {
        icon_pending *pending = link->data;
        if (pending->id == id) {
            g_queue_delete_link(&lookups->pending, link);
            return pending;
        }
    }
    return NULL;
}

static void complete(icon_lookups *lookups, icon_pending *pending, const char *path)
{
    cairo_surface_t *icon = NULL;

    if (path && *path && pending->size > 0) {
        icon = icon_cache_load(lookups->cache, path, pending->size);
    }
    pending->done(pending->context, icon);
    if (icon) {
        cairo_surface_destroy(icon);
    }
    g_free(pending);
}
