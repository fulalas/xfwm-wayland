#include "x11_settings.h"

#include "x11_setting_values.h"

void x11_settings_collect(XfconfChannel *channel, const x11_setting_sink *sink)
{
    GHashTable *properties = channel != NULL ? xfconf_channel_get_properties(channel, NULL) : NULL;
    if (properties == NULL) {
        return;
    }
    GHashTableIter iter;
    gpointer key;
    gpointer value;
    g_hash_table_iter_init(&iter, properties);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        x11_setting setting;
        if (x11_setting_from_property(key, value, &setting)) {
            sink->add(sink->self, &setting);
        }
    }
    g_hash_table_destroy(properties);
}
