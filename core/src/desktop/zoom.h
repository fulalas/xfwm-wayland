#ifndef DESKTOP_ZOOM_H
#define DESKTOP_ZOOM_H

#include <stdbool.h>

#include "wm/desktop.h"

typedef struct zoom_state {
    int inverse_scale;
    wm_point center;
    bool dirty;
} zoom_state;

void zoom_init(wm_desktop *desktop);
void zoom_reset(wm_desktop *desktop);
void zoom_in(wm_desktop *desktop, wm_point pointer);
void zoom_out(wm_desktop *desktop, wm_point pointer);
void zoom_follow_pointer(wm_desktop *desktop, wm_point pointer);
void zoom_present(wm_desktop *desktop);

#endif
