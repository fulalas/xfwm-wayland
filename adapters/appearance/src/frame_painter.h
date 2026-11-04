#ifndef APPEARANCE_FRAME_PAINTER_H
#define APPEARANCE_FRAME_PAINTER_H

#include "compositor/painter.h"
#include "theme.h"
#include "title_text.h"

typedef struct frame_painter {
    const theme *theme;
    const title_style *title;
} frame_painter;

cairo_surface_t *appearance_frame_paint(const frame_painter *painter,
                                        const frame_paint_request *request);

#endif
