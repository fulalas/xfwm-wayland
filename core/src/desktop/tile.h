#ifndef DESKTOP_TILE_H
#define DESKTOP_TILE_H

#include <stdbool.h>

#include "window.h"

bool tile_window_at(wm_desktop *desktop, wm_window *window, wm_point point, tile_mode mode);
void tile_toggle(wm_desktop *desktop, wm_window *window, tile_mode mode);
void tile_update_size(wm_desktop *desktop, wm_window *window);

#endif
