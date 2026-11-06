#ifndef DESKTOP_WRAP_H
#define DESKTOP_WRAP_H

#include <stdint.h>

#include "wm/desktop.h"

typedef struct wrap_state {
    uint32_t last_resist_ms;
    int edge_x;
    int edge_y;
    int original_x;
    int original_y;
} wrap_state;

void wrap_at_screen_edges(wm_desktop *desktop, wm_window *carried, uint32_t time_ms);

#endif
