#ifndef DESKTOP_DIRECTION_H
#define DESKTOP_DIRECTION_H

#include "wm/geometry.h"

typedef enum move_direction {
    DIRECTION_NONE,
    DIRECTION_UP,
    DIRECTION_DOWN,
    DIRECTION_LEFT,
    DIRECTION_RIGHT,
} move_direction;

wm_point direction_offset(move_direction direction);

#endif
