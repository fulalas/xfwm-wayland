#ifndef COMPOSITOR_XCURSOR_RESIZE_H
#define COMPOSITOR_XCURSOR_RESIZE_H

#include <stdint.h>

typedef struct xcursor_pixels {
    const uint32_t *data;
    int width;
    int height;
} xcursor_pixels;

int xcursor_resized_length(int length, int target, int nominal);
void xcursor_resize(const xcursor_pixels *source, uint32_t *target, int width, int height);

#endif
