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

typedef struct grid_selection {
    size_t count;
    size_t current;
} grid_selection;

tabwin_grid tabwin_compute(const wm_desktop *desktop, size_t count);
size_t tabwin_select_delta(const tabwin_grid *grid, grid_selection selection, wm_point move);

#endif
