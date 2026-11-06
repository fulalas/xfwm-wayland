#include "xcursor_resize.h"

#include <math.h>

int xcursor_resized_length(int length, int target, int nominal)
{
    if (nominal <= 0) {
        return length;
    }
    long resized = lround((double) length * target / nominal);
    return resized > 0 ? (int) resized : 1;
}

void xcursor_resize(const xcursor_pixels *source, uint32_t *target, int width, int height)
{
    for (int y = 0; y < height; y++) {
        int source_y = (int) ((long) y * source->height / height);
        for (int x = 0; x < width; x++) {
            int source_x = (int) ((long) x * source->width / width);
            target[(long) y * width + x] = source->data[(long) source_y * source->width + source_x];
        }
    }
}
