#ifndef COMPOSITOR_MAPPING_LINE_H
#define COMPOSITOR_MAPPING_LINE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct mapping {
    uintptr_t start;
    size_t length;
} mapping;

bool mapping_line_parse(const char *maps_line, mapping *mapping);

#endif
