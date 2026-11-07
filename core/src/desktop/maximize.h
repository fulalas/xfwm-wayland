#ifndef DESKTOP_MAXIMIZE_H
#define DESKTOP_MAXIMIZE_H

#include <stdbool.h>
#include <stdint.h>

#include "wm/desktop.h"

bool maximize_toggle(wm_desktop *desktop, wm_window *window, uint32_t mode);
bool maximize_toggle_in_place(wm_desktop *desktop, wm_window *window, uint32_t mode);
bool maximize_toggle_at(wm_desktop *desktop, wm_window *window, wm_point point);
void maximize_save_geometry(wm_window *window);
bool maximize_restore_geometry(wm_window *window);
void maximize_update_size(wm_desktop *desktop, wm_window *window);
void maximize_remove_flag(wm_window *window);

#endif
