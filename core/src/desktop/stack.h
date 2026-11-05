#ifndef DESKTOP_STACK_H
#define DESKTOP_STACK_H

#include <stdbool.h>

#include "wm/desktop.h"

void stack_add(wm_desktop *desktop, wm_window *window);
void stack_remove(wm_desktop *desktop, wm_window *window);
void stack_raise(wm_desktop *desktop, wm_window *window);
void stack_lower(wm_desktop *desktop, wm_window *window);
void stack_update_layers(wm_desktop *desktop);
bool stack_layers_outdated(const wm_desktop *desktop);
void stack_place_below(wm_desktop *desktop, wm_window *window, const wm_window *sibling);
void stack_place_above_parent(wm_desktop *desktop, wm_window *window);
bool stack_is_top_most(const wm_desktop *desktop, const wm_window *window);

#endif
