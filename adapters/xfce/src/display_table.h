#ifndef XFCE_DISPLAY_TABLE_H
#define XFCE_DISPLAY_TABLE_H

#include <glib.h>
#include <stdbool.h>

typedef struct display_key {
    const char *profile;
    const char *connector;
    const char *leaf;
} display_key;

GHashTable *display_table_new(void);
char *display_key_text(const display_key *key);
const char *display_table_text(GHashTable *table, const display_key *key);
bool display_table_bool(GHashTable *table, const display_key *key);
int display_table_int(GHashTable *table, const display_key *key, int fallback);
double display_table_double(GHashTable *table, const display_key *key, double fallback);
char *display_double_text(double value);

#endif
