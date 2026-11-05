#ifndef XFCE_DISPLAY_READ_H
#define XFCE_DISPLAY_READ_H

#include <glib.h>

#include "compositor/displays.h"

typedef struct display_lookup {
    GHashTable *table;
    const char *profile;
    const char *connector;
    char *const *hashes;
} display_lookup;

display_saved display_profile_read(const display_lookup *lookup);

#endif
