#ifndef APPEARANCE_LABEL_PAINTER_H
#define APPEARANCE_LABEL_PAINTER_H

#include "compositor/painter.h"
#include "ui_text.h"

wm_size appearance_label_layout(const text_font *font, const char *text);
cairo_surface_t *appearance_label_paint(const ui_text *ui, const label_paint_request *request);

#endif
