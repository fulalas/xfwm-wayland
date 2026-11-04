#ifndef APPEARANCE_MENU_PAINTER_H
#define APPEARANCE_MENU_PAINTER_H

#include "compositor/painter.h"
#include "ui_text.h"

cairo_surface_t *appearance_menu_paint(const ui_text *ui, const menu_paint_request *request);

#endif
