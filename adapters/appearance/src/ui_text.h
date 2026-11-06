#ifndef APPEARANCE_UI_TEXT_H
#define APPEARANCE_UI_TEXT_H

#include "text.h"

typedef struct ui_text {
    const ui_style *style;
    const text_font *font;
} ui_text;

typedef struct text_box {
    const char *text;
    wm_rect rect;
    const ui_color *color;
    PangoAlignment alignment;
    bool underline;
    int underline_at;
} text_box;

void appearance_ui_text_draw(cairo_t *cr, const ui_text *ui, const text_box *box);
void appearance_ui_fill_rect(cairo_t *cr, const wm_rect *rect, const ui_color *color);
void appearance_ui_panel(cairo_t *cr, const ui_style *style, wm_size size, int border);

#endif
