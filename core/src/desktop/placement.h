#ifndef DESKTOP_PLACEMENT_H
#define DESKTOP_PLACEMENT_H

#include <stdbool.h>

#include "wm/desktop.h"

void placement_init_position(wm_desktop *desktop, wm_window *window);
void placement_place_in(wm_desktop *desktop, wm_window *window, const wm_rect *area);
void placement_anchor(const wm_desktop *desktop, wm_window *window, const wm_rect *area,
                      wm_rule_anchor anchor, int margin);
void placement_constrain(wm_desktop *desktop, wm_window *window);
void placement_constrain_fully(wm_desktop *desktop, wm_window *window);
void placement_smart(const wm_desktop *desktop, wm_window *window, const wm_rect *area);

#endif
