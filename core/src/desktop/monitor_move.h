#ifndef DESKTOP_MONITOR_MOVE_H
#define DESKTOP_MONITOR_MOVE_H

#include <stdbool.h>
#include <stddef.h>

#include "wm/desktop.h"

typedef enum monitor_direction {
    MONITOR_LEFT,
    MONITOR_RIGHT,
    MONITOR_UP,
    MONITOR_DOWN,
} monitor_direction;

bool monitor_move_possible(const wm_desktop *desktop, const wm_window *window,
                           monitor_direction direction);
void monitor_move(wm_desktop *desktop, wm_window *window, monitor_direction direction);
void monitor_move_to(wm_desktop *desktop, wm_window *window, size_t target);
void monitor_move_follow_outputs(wm_desktop *desktop, wm_window *window);

#endif
