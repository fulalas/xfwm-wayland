#include "appearance/appearance.h"

#include <glib.h>

#include "color_symbols.h"

static const char fallback_font[] = "Sans 10";

enum {
    BYTE_MASK = 0xFF,
    RED_SHIFT = 16,
    GREEN_SHIFT = 8,
    COLOR_BACKGROUND = 0xF6F5F4,
    COLOR_FOREGROUND = 0x2E3436,
    COLOR_SELECTED_BACKGROUND = 0x3584E4,
    COLOR_SELECTED_FOREGROUND = 0xFFFFFF,
    COLOR_BORDER = 0xCDC7C2,
    COLOR_INSENSITIVE_FOREGROUND = 0x929595,
};

static ui_color rgb(unsigned hex);

ui_style appearance_fallback_style(void)
{
    ui_style style = {
        .background = rgb(COLOR_BACKGROUND),
        .foreground = rgb(COLOR_FOREGROUND),
        .selected_background = rgb(COLOR_SELECTED_BACKGROUND),
        .selected_foreground = rgb(COLOR_SELECTED_FOREGROUND),
        .border = rgb(COLOR_BORDER),
        .insensitive_foreground = rgb(COLOR_INSENSITIVE_FOREGROUND),
    };

    for (int i = 0; i < APPEARANCE_COLOR_SYMBOL_COUNT; i++) {
        g_strlcpy(style.color_symbols[i], appearance_color_symbol_fallback(i),
                  APPEARANCE_COLOR_TEXT_MAX);
    }
    g_strlcpy(style.font, fallback_font, APPEARANCE_FONT_MAX);
    return style;
}

static ui_color rgb(unsigned hex)
{
    return (ui_color) {
        .red = ((hex >> RED_SHIFT) & BYTE_MASK) / (double) BYTE_MASK,
        .green = ((hex >> GREEN_SHIFT) & BYTE_MASK) / (double) BYTE_MASK,
        .blue = (hex & BYTE_MASK) / (double) BYTE_MASK,
        .alpha = 1.0,
    };
}
