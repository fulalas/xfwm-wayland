#ifndef XFCE_DISPLAY_WRITE_H
#define XFCE_DISPLAY_WRITE_H

#include <glib.h>
#include <stdbool.h>

#include "compositor/displays.h"

typedef enum profile_value_kind {
    PROFILE_VALUE_TEXT,
    PROFILE_VALUE_BOOL,
    PROFILE_VALUE_INT,
    PROFILE_VALUE_DOUBLE,
} profile_value_kind;

typedef struct profile_write {
    char *key;
    profile_value_kind kind;
    char *text;
} profile_write;

GPtrArray *display_profile_writes(const display_set *current);
bool display_profile_is_saved(GHashTable *table, const GPtrArray *writes);

#endif
