#include "mapping_set.h"

void mapping_set_clear(mapping_set *set)
{
    set->count = 0;
}

void mapping_set_add(mapping_set *set, const mapping *mapping)
{
    if (set->count < MAPPING_SET_MAX) {
        set->items[set->count++] = *mapping;
    }
}

bool mapping_set_has(const mapping_set *set, const mapping *mapping)
{
    for (size_t i = 0; i < set->count; i++) {
        if (set->items[i].start == mapping->start && set->items[i].length == mapping->length) {
            return true;
        }
    }
    return false;
}
