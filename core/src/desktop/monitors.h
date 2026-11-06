#ifndef DESKTOP_MONITORS_H
#define DESKTOP_MONITORS_H

#include <stddef.h>

#include "wm/desktop.h"

size_t monitors_count(const wm_desktop *desktop);
wm_rect monitors_full_rect(const wm_desktop *desktop, size_t index);
size_t monitors_index_at(const wm_desktop *desktop, wm_point point);
wm_rect monitors_full_at(const wm_desktop *desktop, wm_point point);
bool monitors_primary_full(const wm_desktop *desktop, wm_rect *full);
wm_rect monitors_work_area_at(const wm_desktop *desktop, wm_point point);
wm_rect monitors_work_area_of(const wm_desktop *desktop, size_t index);
wm_rect monitors_screen_bounds(const wm_desktop *desktop);
wm_point monitors_pointer(const wm_desktop *desktop);

#endif
