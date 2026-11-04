#ifndef DESKTOP_TABWIN_H
#define DESKTOP_TABWIN_H

#include <stdbool.h>
#include <stddef.h>

#include "wm/desktop.h"

typedef struct tabwin_grid {
    wm_tabwin_mode mode;
    bool preview;
    int columns;
    int rows;
    int item_size;
    wm_rect monitor;
} tabwin_grid;

typedef struct grid_move {
    int rows;
    int columns;
} grid_move;

tabwin_grid tabwin_compute(const wm_desktop *desktop, size_t count);
size_t tabwin_select_delta(const tabwin_grid *grid, size_t count, size_t current, grid_move move);

#endif
