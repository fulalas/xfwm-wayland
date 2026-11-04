#ifndef APPEARANCE_TABWIN_PAINTER_H
#define APPEARANCE_TABWIN_PAINTER_H

#include "compositor/painter.h"
#include "ui_text.h"

cairo_surface_t *appearance_tabwin_paint(const ui_text *ui, const cycle_paint_request *request);

#endif
