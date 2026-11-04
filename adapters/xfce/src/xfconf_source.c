#include "xfconf_source.h"

struct xfconf_source {
    XfconfChannel *channel;
    char *prefix;
    GHashTable *texts;
};

static const char *lookup(void *self, const char *key);
static char *read_text(const xfconf_source *source, const char *key);
static char *value_to_text(const GValue *value);
static char *number_to_text(const GValue *value);
static char *array_to_text(const GPtrArray *values);

xfconf_source *xfconf_source_create(XfconfChannel *channel, const char *prefix)
{
    xfconf_source *source = g_new0(xfconf_source, 1);

    source->channel = channel;
    source->prefix = g_strdup(prefix);
    source->texts = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    return source;
}

void xfconf_source_destroy(xfconf_source *source)
{
    g_hash_table_destroy(source->texts);
    g_free(source->prefix);
    g_free(source);
}

setting_source xfconf_source_view(xfconf_source *source)
{
    return (setting_source) { .self = source, .lookup = lookup };
}

static const char *lookup(void *self, const char *key)
{
    xfconf_source *source = self;
    const char *cached = g_hash_table_lookup(source->texts, key);
    char *text;

    if (cached) {
        return cached;
    }
    text = read_text(source, key);
    if (text) {
        g_hash_table_insert(source->texts, g_strdup(key), text);
    }
    return text;
}

static char *read_text(const xfconf_source *source, const char *key)
{
    char *property = g_strconcat(source->prefix, key, NULL);
    GValue value = G_VALUE_INIT;
    char *text = NULL;

    if (source->channel && xfconf_channel_get_property(source->channel, property, &value)) {
        text = value_to_text(&value);
        g_value_unset(&value);
    }
    g_free(property);
    return text;
}

static char *value_to_text(const GValue *value)
{
    if (G_VALUE_HOLDS_STRING(value)) {
        return g_value_dup_string(value);
    }
    if (G_VALUE_HOLDS_BOOLEAN(value)) {
        return g_strdup(g_value_get_boolean(value) ? "true" : "false");
    }
    if (G_VALUE_HOLDS(value, G_TYPE_PTR_ARRAY)) {
        return array_to_text(g_value_get_boxed(value));
    }
    return number_to_text(value);
}

static char *number_to_text(const GValue *value)
{
    GValue number = G_VALUE_INIT;
    char buffer[G_ASCII_DTOSTR_BUF_SIZE];

    if (!g_value_type_transformable(G_VALUE_TYPE(value), G_TYPE_DOUBLE)) {
        return NULL;
    }
    g_value_init(&number, G_TYPE_DOUBLE);
    g_value_transform(value, &number);
    g_ascii_dtostr(buffer, sizeof(buffer), g_value_get_double(&number));
    return g_strdup(buffer);
}

static char *array_to_text(const GPtrArray *values)
{
    GString *text = g_string_new(NULL);

    for (guint i = 0; values && i < values->len; i++) {
        char *item = value_to_text(g_ptr_array_index(values, i));

        if (i > 0) {
            g_string_append_c(text, ',');
        }
        g_string_append(text, item ? item : "");
        g_free(item);
    }
    return g_string_free(text, FALSE);
}
