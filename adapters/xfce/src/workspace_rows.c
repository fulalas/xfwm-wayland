#include "workspace_rows.h"

#include <stdio.h>
#include <string.h>

#include "wm/config.h"

enum { ROWS_KEY_MAX = 48 };

static const char plugin_prefix[] = "/plugins/plugin-";

static int plugin_id(const char *key);
static bool is_pager(const GValue *value);
static int int_of(const GValue *value);

int pager_rows_from(GHashTable *plugins)
{
    GHashTableIter iter;
    void *key;
    void *value;
    int pager = -1;

    g_hash_table_iter_init(&iter, plugins);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        int id = plugin_id(key);
        if (id >= 0 && is_pager(value) && (pager < 0 || id < pager)) {
            pager = id;
        }
    }
    if (pager < 0) {
        return 0;
    }
    char rows_key[ROWS_KEY_MAX];
    snprintf(rows_key, sizeof(rows_key), "%s%d/rows", plugin_prefix, pager);
    const GValue *rows = g_hash_table_lookup(plugins, rows_key);
    return CLAMP(rows ? int_of(rows) : 1, 1, WM_WORKSPACE_MAX);
}

static int plugin_id(const char *key)
{
    if (!g_str_has_prefix(key, plugin_prefix)) {
        return -1;
    }
    const char *digits = key + strlen(plugin_prefix);
    if (*digits == '\0' || digits[strspn(digits, "0123456789")] != '\0') {
        return -1;
    }
    return (int) MIN(g_ascii_strtoll(digits, NULL, 10), G_MAXINT);
}

static bool is_pager(const GValue *value)
{
    return G_VALUE_HOLDS_STRING(value) && g_strcmp0(g_value_get_string(value), "pager") == 0;
}

static int int_of(const GValue *value)
{
    GValue number = G_VALUE_INIT;
    int result = 0;

    g_value_init(&number, G_TYPE_INT);
    if (g_value_transform(value, &number)) {
        result = g_value_get_int(&number);
    }
    g_value_unset(&number);
    return result;
}
