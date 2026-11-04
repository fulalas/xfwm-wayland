#ifndef DESKTOP_CYCLE_LIST_H
#define DESKTOP_CYCLE_LIST_H

#include <stdbool.h>
#include <stddef.h>

#include "wm/desktop.h"

enum cycle_search {
    SEARCH_INCLUDE_HIDDEN = 1 << 0,
    SEARCH_INCLUDE_ALL_WORKSPACES = 1 << 1,
    SEARCH_INCLUDE_SKIP = 1 << 2,
    SEARCH_SAME_APPLICATION = 1 << 3,
    SEARCH_DIFFERENT_APPLICATION = 1 << 4,
    SEARCH_NORMAL_ONLY = 1 << 5,
};

typedef struct cycle_candidate {
    const wm_window *reference;
    unsigned mask;
} cycle_candidate;

unsigned cycle_list_range(const wm_desktop *desktop);
size_t cycle_list_build(const wm_desktop *desktop, wm_window *start, wm_window **items);
bool cycle_list_selects(const wm_desktop *desktop, const wm_window *window,
                        const cycle_candidate *candidate);

#endif
