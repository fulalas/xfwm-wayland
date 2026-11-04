#ifndef DESKTOP_WINDOW_OPS_H
#define DESKTOP_WINDOW_OPS_H

#include <stdint.h>

#include "wm/desktop.h"

void window_ops_close(wm_desktop *desktop, wm_window *window);
void window_ops_hide(wm_desktop *desktop, wm_window *window);
void window_ops_maximize(wm_desktop *desktop, wm_window *window, uint32_t mode);
void window_ops_double_click(wm_desktop *desktop, wm_window *window);
void window_ops_show(wm_desktop *desktop, wm_window *window);
void window_ops_raise_or_lower(wm_desktop *desktop, wm_window *window);
void window_ops_send_to_workspace(wm_desktop *desktop, wm_window *window, int workspace);
void window_ops_toggle_stick(wm_desktop *desktop, wm_window *window);

#endif
