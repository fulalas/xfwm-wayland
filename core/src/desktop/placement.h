#ifndef DESKTOP_PLACEMENT_H
#define DESKTOP_PLACEMENT_H

#include <stdbool.h>

#include "wm/desktop.h"

void placement_init_position(wm_desktop *desktop, wm_window *window);
void placement_constrain(wm_desktop *desktop, wm_window *window);
void placement_constrain_fully(wm_desktop *desktop, wm_window *window);
void placement_smart(const wm_desktop *desktop, wm_window *window, const wm_rect *area);

#endif
