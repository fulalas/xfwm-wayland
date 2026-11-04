#ifndef DESKTOP_VIEW_H
#define DESKTOP_VIEW_H

#include "wm/desktop.h"

void view_refresh(wm_desktop *desktop, wm_window *window);
void view_invalidate_decor(wm_desktop *desktop);

#endif
