#include "name_watches.h"

static void unwatch(gpointer data)
{
    g_bus_unwatch_name(GPOINTER_TO_UINT(data));
}

GHashTable *name_watches_new(void)
{
    return g_hash_table_new_full(g_str_hash, g_str_equal, g_free, unwatch);
}

void name_watches_add(GHashTable *watches, GDBusConnection *connection, const char *name,
                      GBusNameVanishedCallback vanished, gpointer data)
{
    if (name == NULL || g_hash_table_contains(watches, name)) {
        return;
    }
    guint watch = g_bus_watch_name_on_connection(connection, name, G_BUS_NAME_WATCHER_FLAGS_NONE,
                                                 NULL, vanished, data, NULL);
    g_hash_table_insert(watches, g_strdup(name), GUINT_TO_POINTER(watch));
}
