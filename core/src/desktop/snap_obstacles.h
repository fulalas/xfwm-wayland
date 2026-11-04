#ifndef DESKTOP_SNAP_OBSTACLES_H
#define DESKTOP_SNAP_OBSTACLES_H

#include <stddef.h>

#include "wm/desktop.h"

typedef struct snap_obstacles {
    wm_rect *rects;
    size_t count;
} snap_obstacles;

snap_obstacles snap_obstacles_collect(const wm_desktop *desktop, const wm_window *window);
void snap_obstacles_free(snap_obstacles *obstacles);

#endif
