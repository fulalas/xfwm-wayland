#ifndef DESKTOP_FILL_H
#define DESKTOP_FILL_H

#include "wm/desktop.h"

enum fill_mode {
    FILL_VERTICAL = 1 << 0,
    FILL_HORIZONTAL = 1 << 1,
    FILL_BOTH = FILL_VERTICAL | FILL_HORIZONTAL,
};

void fill_window(wm_desktop *desktop, wm_window *window, unsigned mode);

#endif
