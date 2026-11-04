#ifndef COMPOSITOR_DRAG_ACTIONS_H
#define COMPOSITOR_DRAG_ACTIONS_H

#include <stdbool.h>
#include <stdint.h>

typedef struct drag_start {
    int32_t source_actions;
    uint32_t button;
    bool alt_held;
} drag_start;

int32_t drag_actions_offered(const drag_start *start);

#endif
