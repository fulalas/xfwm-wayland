#ifndef DESKTOP_SIZE_HINTS_H
#define DESKTOP_SIZE_HINTS_H

#include "wm/desktop.h"

void size_hints_normalize(wm_size_hints *hints);
int size_hints_check_width(const wm_desktop *desktop, const wm_window *window, int width);
int size_hints_check_height(const wm_desktop *desktop, const wm_window *window, int height);
void size_hints_fit_user(const wm_desktop *desktop, wm_window *window, uint32_t edges);
bool size_hints_allow(const wm_window *window, wm_size size);
bool size_hints_has_steps(const wm_window *window);
wm_size size_hints_steps_of(const wm_window *window);
wm_point size_hints_key_steps(const wm_window *window, int nominal);

#endif
