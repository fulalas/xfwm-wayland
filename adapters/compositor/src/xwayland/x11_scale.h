#ifndef COMPOSITOR_X11_SCALE_H
#define COMPOSITOR_X11_SCALE_H

#include "wm/geometry.h"

int x11_scale_to_x(double scale, int logical);
int x11_scale_from_x(double scale, int x);
wm_rect x11_scale_rect_to_x(double scale, wm_rect logical);
wm_rect x11_scale_rect_from_x(double scale, wm_rect x);
wm_size x11_scale_size_from_x(double scale, wm_size x);
wm_size x11_scale_size_from_x_up(double scale, wm_size x);
wm_size x11_scale_size_from_x_down(double scale, wm_size x);
wm_extents x11_scale_extents_to_x(double scale, wm_extents logical);

#endif
