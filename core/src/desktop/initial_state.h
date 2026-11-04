#ifndef DESKTOP_INITIAL_STATE_H
#define DESKTOP_INITIAL_STATE_H

#include "wm/desktop.h"

void initial_state_apply(wm_desktop *desktop, wm_window *window);
void initial_state_show(wm_desktop *desktop, wm_window *window);

#endif
