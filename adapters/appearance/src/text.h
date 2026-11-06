#ifndef APPEARANCE_TEXT_H
#define APPEARANCE_TEXT_H

#include <pango/pangocairo.h>

#include "appearance/appearance.h"
#include "wm/geometry.h"

typedef struct text_font {
    PangoContext *context;
    PangoFontDescription *description;
    int line_height;
} text_font;

typedef struct text_font_request {
    const char *description;
    double dpi;
} text_font_request;

text_font appearance_text_font_create(const text_font_request *request);
void appearance_text_font_destroy(text_font *font);
PangoLayout *appearance_text_layout(const text_font *font, const char *text);
wm_size appearance_text_layout_size(PangoLayout *layout);
int appearance_text_line_height(const text_font *font);
void appearance_text_set_color(cairo_t *cr, const ui_color *color);

#endif
