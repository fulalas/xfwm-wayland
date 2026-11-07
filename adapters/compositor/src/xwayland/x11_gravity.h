#ifndef COMPOSITOR_X11_GRAVITY_H
#define COMPOSITOR_X11_GRAVITY_H

#include <stdint.h>

#include "wm/geometry.h"

typedef struct x11_configure {
    wm_rect values;
    uint32_t fields;
    int gravity;
} x11_configure;

typedef struct x11_framed_window {
    wm_rect client;
    wm_extents extents;
} x11_framed_window;

wm_rect x11_gravity_configure(const x11_framed_window *window, const x11_configure *request);
wm_point x11_gravity_place(wm_point reference, int gravity, const wm_extents *extents);

#endif
