#include "icon_pixels.h"

enum {
    CHANNEL_MAX = 255,
    ALPHA_SHIFT = 24,
    RED_SHIFT = 16,
    GREEN_SHIFT = 8,
    CHANNEL_MASK = 0xff,
};

static uint32_t scale_channel(uint32_t argb, int shift, uint32_t alpha);
static int size_penalty(const wm_size *size, int wanted);

uint32_t icon_pixels_premultiply(uint32_t argb)
{
    uint32_t alpha = (argb >> ALPHA_SHIFT) & CHANNEL_MASK;
    return (alpha << ALPHA_SHIFT) | (scale_channel(argb, RED_SHIFT, alpha) << RED_SHIFT) |
           (scale_channel(argb, GREEN_SHIFT, alpha) << GREEN_SHIFT) | scale_channel(argb, 0, alpha);
}

size_t icon_pixels_best_index(const wm_size *sizes, size_t count, int wanted)
{
    size_t best = 0;
    for (size_t i = 1; i < count; i++) {
        if (size_penalty(&sizes[i], wanted) < size_penalty(&sizes[best], wanted)) {
            best = i;
        }
    }
    return best;
}

static uint32_t scale_channel(uint32_t argb, int shift, uint32_t alpha)
{
    uint32_t channel = (argb >> shift) & CHANNEL_MASK;
    return (channel * alpha + CHANNEL_MAX / 2) / CHANNEL_MAX;
}

static int size_penalty(const wm_size *size, int wanted)
{
    int edge = size->width > size->height ? size->width : size->height;
    if (edge >= wanted) {
        return edge - wanted;
    }
    return (wanted - edge) * CHANNEL_MAX;
}
