#ifndef DESKTOP_CURSOR_H
#define DESKTOP_CURSOR_H

#include "wm/desktop.h"

void cursor_show_session(wm_desktop *desktop);
void cursor_show_hover(wm_desktop *desktop, const wm_window *window, wm_part part);

#endif
