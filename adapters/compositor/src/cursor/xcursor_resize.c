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

void xcursor_resize(const xcursor_pixels *source, uint32_t *target, wm_size size)
{
    wm_size from = source->size;
    for (int y = 0; y < size.height; y++) {
        int source_y = (int) ((long) y * from.height / size.height);
        for (int x = 0; x < size.width; x++) {
            int source_x = (int) ((long) x * from.width / size.width);
            long from_index = (long) source_y * from.width + source_x;
            target[(long) y * size.width + x] = source->data[from_index];
        }
    }
}
