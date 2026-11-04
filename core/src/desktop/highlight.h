#ifndef DESKTOP_HIGHLIGHT_H
#define DESKTOP_HIGHLIGHT_H

#include "wm/desktop.h"

void highlight_begin(wm_desktop *desktop, wm_window *window);
void highlight_end(wm_desktop *desktop, const wm_window *window);
void highlight_present(wm_desktop *desktop);

#endif
