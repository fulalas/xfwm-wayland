#ifndef DESKTOP_STATES_H
#define DESKTOP_STATES_H

#include "wm/desktop.h"

void states_shade(wm_desktop *desktop, wm_window *window);
void states_unshade(wm_desktop *desktop, wm_window *window);
void states_toggle_shade(wm_desktop *desktop, wm_window *window);
void states_stick(wm_desktop *desktop, wm_window *window);
void states_unstick(wm_desktop *desktop, wm_window *window);
void states_toggle_stick(wm_desktop *desktop, wm_window *window);
void states_toggle_above(wm_desktop *desktop, wm_window *window);
void states_toggle_below(wm_desktop *desktop, wm_window *window);
void states_set_normal_layer(wm_desktop *desktop, wm_window *window);
void states_enter_fullscreen(wm_desktop *desktop, wm_window *window);
void states_leave_fullscreen(wm_desktop *desktop, wm_window *window);
void states_toggle_fullscreen(wm_desktop *desktop, wm_window *window);
void states_update_fullscreen_size(wm_desktop *desktop, wm_window *window);

#endif
