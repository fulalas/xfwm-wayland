#ifndef DESKTOP_VISIBILITY_H
#define DESKTOP_VISIBILITY_H

#include <stdbool.h>

#include "wm/desktop.h"

bool visibility_on_current_workspace(const wm_desktop *desktop, const wm_window *window);
void visibility_show(wm_desktop *desktop, wm_window *window);
void visibility_unminimize(wm_desktop *desktop, wm_window *window);
void visibility_hide(wm_desktop *desktop, wm_window *window);
void visibility_minimize(wm_desktop *desktop, wm_window *window);
void visibility_minimize_others(wm_desktop *desktop, wm_window *window);
void visibility_set_workspace(wm_desktop *desktop, wm_window *window, int workspace);
void visibility_assign_workspace(wm_desktop *desktop, wm_window *window, int workspace);
void visibility_clear_show_desktop(wm_desktop *desktop);
void visibility_toggle_show_desktop(wm_desktop *desktop);

#endif
