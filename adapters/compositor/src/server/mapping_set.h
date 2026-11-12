#ifndef COMPOSITOR_MAPPING_SET_H
#define COMPOSITOR_MAPPING_SET_H

#include <stdbool.h>
#include <stddef.h>

#include "mapping_line.h"

enum {
    MAPPING_SET_MAX = 64,
};

typedef struct mapping_set {
    mapping items[MAPPING_SET_MAX];
    size_t count;
} mapping_set;

void mapping_set_clear(mapping_set *set);
void mapping_set_add(mapping_set *set, const mapping *mapping);
bool mapping_set_has(const mapping_set *set, const mapping *mapping);

#endif
