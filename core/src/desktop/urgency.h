#ifndef DESKTOP_URGENCY_H
#define DESKTOP_URGENCY_H

#include "wm/desktop.h"

void urgency_set(wm_desktop *desktop, wm_window *window);
void urgency_clear(wm_desktop *desktop, wm_window *window);
void urgency_refresh(wm_desktop *desktop, wm_window *window);
void urgency_on_blink(wm_desktop *desktop, wm_window *window);

#endif
