#include "display_table.h"

#include <errno.h>
#include <limits.h>
#include <stdlib.h>

#include "input_text.h"

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
    optional_bool value = input_bool_from_text(display_table_text(table, key));

    return value.is_set && value.value;
}

int display_table_int(GHashTable *table, const display_key *key, int fallback)
{
    const char *value = display_table_text(table, key);
    char *end = NULL;
    long number;

    if (!value) {
        return fallback;
    }
    errno = 0;
    number = strtol(value, &end, 10);
    if (end == value || *end != '\0' || errno == ERANGE || number < INT_MIN || number > INT_MAX) {
        return fallback;
    }
    return (int) number;
}

double display_table_double(GHashTable *table, const display_key *key, double fallback)
{
    optional_double value = input_double_from_text(display_table_text(table, key));

    return value.is_set ? value.value : fallback;
}
