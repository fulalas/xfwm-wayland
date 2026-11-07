#ifndef COMPOSITOR_ICON_PIXELS_H
#define COMPOSITOR_ICON_PIXELS_H

#include <stddef.h>
#include <stdint.h>

#include "wm/geometry.h"

uint32_t icon_pixels_premultiply(uint32_t argb);
size_t icon_pixels_best_index(const wm_size *sizes, size_t count, int wanted);

#endif
