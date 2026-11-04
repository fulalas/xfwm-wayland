#ifndef APPEARANCE_TITLE_TEXT_H
#define APPEARANCE_TITLE_TEXT_H

#include "compositor/painter.h"
#include "text.h"

typedef struct title_style {
    text_font font;
    ui_color text[WM_FRAME_STATE_COUNT];
    ui_color shadow[WM_FRAME_STATE_COUNT];
} title_style;

wm_size appearance_title_measure(const title_style *style, const char *title);
void appearance_title_draw(cairo_t *cr, const title_style *style,
                           const frame_paint_request *request);

#endif
