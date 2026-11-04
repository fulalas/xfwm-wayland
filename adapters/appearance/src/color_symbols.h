#ifndef APPEARANCE_COLOR_SYMBOLS_H
#define APPEARANCE_COLOR_SYMBOLS_H

#include "appearance/appearance.h"
#include "themerc.h"
#include "xpm.h"

enum {
    COLOR_SYMBOL_VALUE_MAX = 129,
};

typedef enum color_symbol_id {
    SYMBOL_ACTIVE_TEXT = 0,
    SYMBOL_INACTIVE_TEXT = 1,
    SYMBOL_ACTIVE_TEXT_SHADOW = 2,
    SYMBOL_INACTIVE_TEXT_SHADOW = 3,
} color_symbol_id;

typedef struct color_symbol_values {
    char value[APPEARANCE_COLOR_SYMBOL_COUNT][COLOR_SYMBOL_VALUE_MAX];
    xpm_symbol items[APPEARANCE_COLOR_SYMBOL_COUNT];
} color_symbol_values;

void appearance_color_symbols_resolve(color_symbol_values *values, const ui_style *style,
                                      const themerc *rc);
xpm_symbols appearance_color_symbols_view(const color_symbol_values *values);
const char *appearance_color_symbol_fallback(int index);

#endif
