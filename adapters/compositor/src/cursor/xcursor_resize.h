#ifndef COMPOSITOR_XCURSOR_RESIZE_H
#define COMPOSITOR_XCURSOR_RESIZE_H

#include <stdint.h>

#include "wm/geometry.h"

typedef struct xcursor_pixels {
    const uint32_t *data;
    wm_size size;
} xcursor_pixels;

int xcursor_resized_length(int length, int target, int nominal);
void xcursor_resize(const xcursor_pixels *source, uint32_t *target, wm_size size);

#endif
