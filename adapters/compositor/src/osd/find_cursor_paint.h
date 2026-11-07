#ifndef COMPOSITOR_FIND_CURSOR_PAINT_H
#define COMPOSITOR_FIND_CURSOR_PAINT_H

#include <cairo.h>

#include "find_cursor_rings.h"

cairo_surface_t *find_cursor_paint(const find_cursor_frame *frame, double scale);

#endif
