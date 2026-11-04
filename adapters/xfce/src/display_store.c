#include "display_store.h"

#include "display_choice.h"
#include "display_plan.h"
#include "display_table.h"
#include "display_write.h"

static const char active_profile_property[] = "/ActiveProfile";

static GHashTable *read_table(XfconfChannel *channel);
static void add_text(void *key, void *value, void *table);
static char *value_text(const GValue *value);
static void replace_default_profile(XfconfChannel *channel, const GPtrArray *writes);
static void store(XfconfChannel *channel, const profile_write *write);

display_plan display_store_load(XfconfChannel *channel, const display_set *connected)
{
    GHashTable *table = read_table(channel);
    display_plan plan = display_profile_plan(table, connected);

    g_hash_table_destroy(table);
    return plan;
}

void display_store_save(XfconfChannel *channel, const display_set *current)
{
    GPtrArray *writes;
    GHashTable *table;

    if (!channel) {
        return;
    }
    writes = display_profile_writes(current);
    table = read_table(channel);
    if (!display_profile_is_saved(table, writes)) {
        replace_default_profile(channel, writes);
    }
    g_hash_table_destroy(table);
    g_ptr_array_unref(writes);
}

static GHashTable *read_table(XfconfChannel *channel)
{
    GHashTable *table = display_table_new();
    GHashTable *properties = channel ? xfconf_channel_get_properties(channel, NULL) : NULL;

    if (properties) {
        g_hash_table_foreach(properties, add_text, table);
        g_hash_table_destroy(properties);
    }
    return table;
}

static void add_text(void *key, void *value, void *table)
{
    char *text = value_text(value);

    if (text) {
        g_hash_table_insert(table, g_strdup(key), text);
    }
}

static char *value_text(const GValue *value)
{
    if (G_VALUE_HOLDS_STRING(value)) {
        return g_value_dup_string(value);
    }
    if (G_VALUE_HOLDS_BOOLEAN(value)) {
        return g_strdup(g_value_get_boolean(value) ? "true" : "false");
    }
    if (G_VALUE_HOLDS_INT(value)) {
        return g_strdup_printf("%d", g_value_get_int(value));
    }
    if (G_VALUE_HOLDS_UINT(value)) {
        return g_strdup_printf("%u", g_value_get_uint(value));
    }
    if (G_VALUE_HOLDS_DOUBLE(value)) {
        return display_double_text(g_value_get_double(value));
    }
    return NULL;
}

static void replace_default_profile(XfconfChannel *channel, const GPtrArray *writes)
{
    char *root = g_strdup_printf("/%s", display_default_profile);

    xfconf_channel_reset_property(channel, root, TRUE);
    for (guint i = 0; i < writes->len; i++) {
        store(channel, g_ptr_array_index(writes, i));
    }
    xfconf_channel_set_string(channel, active_profile_property, display_default_profile);
    g_free(root);
}

static void store(XfconfChannel *channel, const profile_write *write)
{
    gint32 number = (gint32) g_ascii_strtoll(write->text, NULL, 10);

    switch (write->kind) {
    case PROFILE_VALUE_BOOL:
        xfconf_channel_set_bool(channel, write->key, g_strcmp0(write->text, "true") == 0);
        break;
    case PROFILE_VALUE_INT:
        xfconf_channel_set_int(channel, write->key, number);
        break;
    case PROFILE_VALUE_DOUBLE:
        xfconf_channel_set_double(channel, write->key, g_ascii_strtod(write->text, NULL));
        break;
    default:
        xfconf_channel_set_string(channel, write->key, write->text);
        break;
    }
}
