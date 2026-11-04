#ifndef DESKTOP_SIZE_HINTS_H
#define DESKTOP_SIZE_HINTS_H

#include "wm/desktop.h"

int size_hints_check_width(const wm_desktop *desktop, const wm_window *window, int width);
int size_hints_check_height(const wm_desktop *desktop, const wm_window *window, int height);
bool size_hints_allow(const wm_window *window, wm_size size);

#endif
