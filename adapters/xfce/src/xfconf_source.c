#include "xfconf_source.h"

#include <string.h>

#include "value_text.h"

struct xfconf_source {
    GHashTable *texts;
};

static GHashTable *read_properties(XfconfChannel *channel, const char *prefix);
static void add_texts(GHashTable *texts, GHashTable *properties, const char *prefix);
static const char *lookup(void *self, const char *key);

xfconf_source *xfconf_source_create(XfconfChannel *channel, const char *prefix)
{
    xfconf_source *source = g_new0(xfconf_source, 1);
    GHashTable *properties = read_properties(channel, prefix);

    source->texts = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    if (properties) {
        add_texts(source->texts, properties, prefix);
        g_hash_table_destroy(properties);
    }
    return source;
}

void xfconf_source_destroy(xfconf_source *source)
{
    g_hash_table_destroy(source->texts);
    g_free(source);
}

setting_source xfconf_source_view(xfconf_source *source)
{
    return (setting_source) { .self = source, .lookup = lookup };
}

static GHashTable *read_properties(XfconfChannel *channel, const char *prefix)
{
    char *base;
    GHashTable *properties;

    if (!channel) {
        return NULL;
    }
    base = g_strndup(prefix, strlen(prefix) - (g_str_has_suffix(prefix, "/") ? 1 : 0));
    properties = xfconf_channel_get_properties(channel, *base ? base : NULL);
    g_free(base);
    return properties;
}

static void add_texts(GHashTable *texts, GHashTable *properties, const char *prefix)
{
    GHashTableIter iter;
    gpointer property;
    gpointer value;
    size_t prefix_length = strlen(prefix);

    g_hash_table_iter_init(&iter, properties);
    while (g_hash_table_iter_next(&iter, &property, &value)) {
        char *text = g_str_has_prefix(property, prefix) ? value_text_of(value) : NULL;
        if (text) {
            g_hash_table_insert(texts, g_strdup((const char *) property + prefix_length), text);
        }
    }
}

static const char *lookup(void *self, const char *key)
{
    xfconf_source *source = self;

    return g_hash_table_lookup(source->texts, key);
}
