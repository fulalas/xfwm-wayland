#ifndef COMPOSITOR_FRAME_BANDS_H
#define COMPOSITOR_FRAME_BANDS_H

#include "wm/decor.h"
#include "wm/geometry.h"

typedef enum frame_band {
    FRAME_BAND_TOP = 0,
    FRAME_BAND_BOTTOM,
    FRAME_BAND_LEFT,
    FRAME_BAND_RIGHT,
    FRAME_BAND_COUNT
} frame_band;

typedef struct frame_bands {
    wm_rect rects[FRAME_BAND_COUNT];
} frame_bands;

frame_bands frame_bands_compute(const wm_decor_layout *layout);

#endif
