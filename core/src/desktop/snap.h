#ifndef DESKTOP_SNAP_H
#define DESKTOP_SNAP_H

#include "wm/desktop.h"

void snap_position(const wm_desktop *desktop, wm_window *window, wm_point previous);
int snap_closest_edge_x(const wm_desktop *desktop, const wm_window *window, int edge);
int snap_closest_edge_y(const wm_desktop *desktop, const wm_window *window, int edge);

#endif
