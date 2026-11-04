#include "display_table.h"

#include <stdlib.h>
#include <string.h>

GHashTable *display_table_new(void)
{
    return g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
}

char *display_key_text(const display_key *key)
{
    if (!key->connector) {
        return g_strdup_printf("/%s", key->profile);
    }
    if (!key->leaf) {
        return g_strdup_printf("/%s/%s", key->profile, key->connector);
    }
    return g_strdup_printf("/%s/%s/%s", key->profile, key->connector, key->leaf);
}

const char *display_table_text(GHashTable *table, const display_key *key)
{
    char *text = display_key_text(key);
    const char *value = g_hash_table_lookup(table, text);

    g_free(text);
    return value;
}

bool display_table_bool(GHashTable *table, const display_key *key)
{
    const char *value = display_table_text(table, key);

    return value && strcmp(value, "true") == 0;
}

int display_table_int(GHashTable *table, const display_key *key, int fallback)
{
    const char *value = display_table_text(table, key);
    char *end = NULL;
    long number;

    if (!value) {
        return fallback;
    }
    number = strtol(value, &end, 10);
    return end != value && *end == '\0' ? (int) number : fallback;
}

double display_table_double(GHashTable *table, const display_key *key, double fallback)
{
    const char *value = display_table_text(table, key);
    char *end = NULL;
    double number;

    if (!value) {
        return fallback;
    }
    number = g_ascii_strtod(value, &end);
    return end != value && *end == '\0' ? number : fallback;
}

char *display_double_text(double value)
{
    char buffer[G_ASCII_DTOSTR_BUF_SIZE];

    return g_strdup(g_ascii_dtostr(buffer, sizeof(buffer), value));
}
