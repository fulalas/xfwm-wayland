#ifndef APPEARANCE_APPEARANCE_H
#define APPEARANCE_APPEARANCE_H

#include <stdbool.h>

#include "compositor/painter.h"
#include "wm/decor.h"

enum {
    APPEARANCE_COLOR_SYMBOL_COUNT = 24,
    APPEARANCE_COLOR_TEXT_MAX = 32,
    APPEARANCE_FONT_MAX = 128,
};

typedef struct ui_color {
    double red;
    double green;
    double blue;
    double alpha;
} ui_color;

typedef struct ui_style {
    char color_symbols[APPEARANCE_COLOR_SYMBOL_COUNT][APPEARANCE_COLOR_TEXT_MAX];
    ui_color background;
    ui_color foreground;
    ui_color selected_background;
    ui_color selected_foreground;
    ui_color border;
    ui_color insensitive_foreground;
    char font[APPEARANCE_FONT_MAX];
} ui_style;

typedef struct theme_request {
    const char *name;
    const char *title_font;
    double max_output_scale;
    const ui_style *style;
} theme_request;

typedef struct appearance appearance;

appearance *appearance_create(void);
void appearance_destroy(appearance *appearance);
bool appearance_load(appearance *appearance, const theme_request *request);
bool appearance_is_loaded(const appearance *appearance, const theme_request *request);
const char *appearance_theme_option(const appearance *appearance, const char *key);
wm_theme_metrics appearance_metrics(const appearance *appearance);
compositor_painter appearance_painter(appearance *appearance);
const char *appearance_color_symbol_name(int index);
ui_style appearance_fallback_style(void);

#endif
