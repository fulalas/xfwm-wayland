#include "direction.h"

wm_point direction_offset(move_direction direction)
{
    static const wm_point offsets[] = {
        [DIRECTION_NONE] = {0, 0},
        [DIRECTION_UP] = {0, -1},
        [DIRECTION_DOWN] = {0, 1},
        [DIRECTION_LEFT] = {-1, 0},
        [DIRECTION_RIGHT] = {1, 0},
    };
    return offsets[direction];
}
