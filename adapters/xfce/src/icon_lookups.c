#include "icon_lookups.h"

#include <glib.h>

#include "dispatcher.h"
#include "icon_cache.h"

enum {
    ANSWERS_READY = 1,
    KNOWN_PATHS_MAX = 256,
};

typedef struct icon_pending {
    unsigned id;
    int size;
    void (*done)(void *context, cairo_surface_t *icon);
    void *context;
    char *key;
    char *known_path;
} icon_pending;

struct icon_lookups {
    GQueue pending;
    GQueue answered;
    unsigned next_id;
    icon_cache *cache;
    GHashTable *paths;
    dispatcher *answers;
};

static icon_pending *new_pending(icon_lookups *lookups, const icon_request *request);
static void free_pending(void *data);
static const char *known_path(icon_lookups *lookups, const char *key);
static void remember_path(icon_lookups *lookups, icon_pending *pending, const char *path);
static void forget_key(void *data, void *unused);
static void cancel_in(GQueue *queue, void *context);
static icon_pending *take_pending(icon_lookups *lookups, unsigned id);
static void on_answers_ready(void *self, unsigned flags);
static void complete(icon_lookups *lookups, icon_pending *pending, const char *path);

icon_lookups *icon_lookups_create(void)
{
    icon_lookups *lookups = g_new0(icon_lookups, 1);
    dispatch_target target = { .self = lookups, .run = on_answers_ready };

    g_queue_init(&lookups->pending);
    g_queue_init(&lookups->answered);
    lookups->cache = icon_cache_create();
    lookups->paths = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    lookups->answers = dispatcher_create(&target);
    return lookups;
}

void icon_lookups_destroy(icon_lookups *lookups)
{
    dispatcher_destroy(lookups->answers);
    g_queue_clear_full(&lookups->pending, free_pending);
    g_queue_clear_full(&lookups->answered, free_pending);
    g_hash_table_destroy(lookups->paths);
    icon_cache_destroy(lookups->cache);
    g_free(lookups);
}

unsigned icon_lookups_add(icon_lookups *lookups, const icon_request *request)
{
    icon_pending *pending = new_pending(lookups, request);
    const char *path = known_path(lookups, pending->key);

    if (!path) {
        g_queue_push_tail(&lookups->pending, pending);
        return pending->id;
    }
    pending->known_path = g_strdup(path);
    g_queue_push_tail(&lookups->answered, pending);
    dispatcher_post(lookups->answers, ANSWERS_READY);
    return ICON_LOOKUP_KNOWN;
}

void icon_lookups_cancel(icon_lookups *lookups, void *context)
{
    cancel_in(&lookups->pending, context);
    cancel_in(&lookups->answered, context);
}

void icon_lookups_finish(icon_lookups *lookups, unsigned id, const char *path)
{
    icon_pending *pending = take_pending(lookups, id);

    if (!pending) {
        return;
    }
    remember_path(lookups, pending, path);
    complete(lookups, pending, path);
}

void icon_lookups_forget_paths(icon_lookups *lookups)
{
    g_hash_table_remove_all(lookups->paths);
    g_queue_foreach(&lookups->pending, forget_key, NULL);
}

static icon_pending *new_pending(icon_lookups *lookups, const icon_request *request)
{
    icon_pending *pending = g_new0(icon_pending, 1);

    *pending = (icon_pending) {
        .id = ++lookups->next_id,
        .size = request->size,
        .done = request->done,
        .context = request->context,
        .key = g_strdup_printf("%d\t%s\t%s", request->size,
                               request->app_id ? request->app_id : "",
                               request->icon_name ? request->icon_name : ""),
    };
    return pending;
}

static void free_pending(void *data)
{
    icon_pending *pending = data;

    g_free(pending->key);
    g_free(pending->known_path);
    g_free(pending);
}

static const char *known_path(icon_lookups *lookups, const char *key)
{
    const char *path = g_hash_table_lookup(lookups->paths, key);

    return path && g_file_test(path, G_FILE_TEST_EXISTS) ? path : NULL;
}

static void remember_path(icon_lookups *lookups, icon_pending *pending, const char *path)
{
    if (!pending->key || !path || !*path) {
        return;
    }
    if (g_hash_table_size(lookups->paths) >= KNOWN_PATHS_MAX) {
        g_hash_table_remove_all(lookups->paths);
    }
    g_hash_table_insert(lookups->paths, g_steal_pointer(&pending->key), g_strdup(path));
}

static void forget_key(void *data, void *unused)
{
    icon_pending *pending = data;

    (void) unused;
    g_clear_pointer(&pending->key, g_free);
}

static void cancel_in(GQueue *queue, void *context)
{
    GList *link = queue->head;

    while (link) {
        GList *next = link->next;
        icon_pending *pending = link->data;
        if (pending->context == context) {
            free_pending(pending);
            g_queue_delete_link(queue, link);
        }
        link = next;
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

static void on_answers_ready(void *self, unsigned flags)
{
    icon_lookups *lookups = self;
    icon_pending *pending;

    (void) flags;
    while ((pending = g_queue_pop_head(&lookups->answered))) {
        complete(lookups, pending, pending->known_path);
    }
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
    free_pending(pending);
}
