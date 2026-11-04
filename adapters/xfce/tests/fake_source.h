#ifndef XFCE_FAKE_SOURCE_H
#define XFCE_FAKE_SOURCE_H

#include <string.h>

#include "setting_source.h"

typedef struct fake_entry {
    const char *key;
    const char *value;
} fake_entry;

static inline const char *fake_lookup(void *self, const char *key)
{
    const fake_entry *entries = self;

    for (size_t i = 0; entries[i].key; i++) {
        if (strcmp(entries[i].key, key) == 0) {
            return entries[i].value;
        }
    }
    return NULL;
}

static inline setting_source fake_source(const fake_entry *entries)
{
    return (setting_source) { .self = (void *) entries, .lookup = fake_lookup };
}

#endif
