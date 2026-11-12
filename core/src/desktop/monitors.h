#ifndef DESKTOP_MONITORS_H
#define DESKTOP_MONITORS_H

#include <stdbool.h>
#include <stddef.h>

#include "wm/desktop.h"

enum {
    MONITORS_REMEMBERED_MAX = 16,
};

typedef struct monitor_memory {
    wm_rect full[MONITORS_REMEMBERED_MAX];
    wm_rect work[MONITORS_REMEMBERED_MAX];
    size_t count;
} monitor_memory;

size_t monitors_count(const wm_desktop *desktop);
wm_rect monitors_full_rect(const wm_desktop *desktop, size_t index);
size_t monitors_index_at(const wm_desktop *desktop, wm_point point);
size_t monitors_index_of(const wm_desktop *desktop, const wm_window *window);
bool monitors_cover(const wm_desktop *desktop, wm_point point);
bool monitors_overlap(const wm_desktop *desktop, const wm_rect *full);
wm_rect monitors_full_at(const wm_desktop *desktop, wm_point point);
bool monitors_primary_full(const wm_desktop *desktop, wm_rect *full);
wm_rect monitors_work_area_at(const wm_desktop *desktop, wm_point point);
wm_rect monitors_work_area_of(const wm_desktop *desktop, size_t index);
wm_rect monitors_screen_bounds(const wm_desktop *desktop);
wm_point monitors_pointer(const wm_desktop *desktop);
void monitors_remember(wm_desktop *desktop);
bool monitors_remembered_at(const wm_desktop *desktop, wm_point point, wm_rect *full,
                            wm_rect *work);
bool monitors_find(const wm_desktop *desktop, const wm_rect *full, size_t *index);
bool monitors_find_named(const wm_desktop *desktop, const char *name, size_t *index);

#endif
