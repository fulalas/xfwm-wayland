#ifndef COMPOSITOR_SHADOW_LAYOUT_H
#define COMPOSITOR_SHADOW_LAYOUT_H

#include "wm/geometry.h"

typedef enum shadow_part {
    SHADOW_TOP_LEFT = 0,
    SHADOW_TOP,
    SHADOW_TOP_RIGHT,
    SHADOW_LEFT,
    SHADOW_MIDDLE,
    SHADOW_RIGHT,
    SHADOW_BOTTOM_LEFT,
    SHADOW_BOTTOM,
    SHADOW_BOTTOM_RIGHT,
    SHADOW_PART_COUNT
} shadow_part;

typedef struct shadow_piece {
    wm_rect source;
    wm_rect dest;
} shadow_piece;

typedef struct shadow_geometry {
    wm_size size;
    int corner;
} shadow_geometry;

typedef struct shadow_pieces {
    shadow_piece pieces[SHADOW_PART_COUNT];
} shadow_pieces;

shadow_pieces shadow_layout_compute(const wm_rect *rect, const shadow_geometry *image);

#endif
