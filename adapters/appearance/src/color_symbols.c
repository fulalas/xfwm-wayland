#include "color_symbols.h"

#include <glib.h>

typedef struct symbol_entry {
    const char *name;
    const char *fallback;
} symbol_entry;

static const symbol_entry symbol_table[APPEARANCE_COLOR_SYMBOL_COUNT] = {
    { "active_text_color", "#ffffff" },
    { "inactive_text_color", "#929595" },
    { "active_text_shadow_color", "#2d5d96" },
    { "inactive_text_shadow_color", "#b7aea4" },
    { "active_border_color", "#2e3436" },
    { "inactive_border_color", "#2e3436" },
    { "active_color_1", "#3584e4" },
    { "active_hilight_1", "#6eaffe" },
    { "active_shadow_1", "#2d5d96" },
    { "active_mid_1", "#4e86ca" },
    { "active_text_color_2", "#2e3436" },
    { "active_color_2", "#f6f5f4" },
    { "active_hilight_2", "#ffffff" },
    { "active_shadow_2", "#b1aba5" },
    { "active_mid_2", "#d8d5d2" },
    { "inactive_color_1", "#faf9f8" },
    { "inactive_hilight_1", "#ffffff" },
    { "inactive_shadow_1", "#b7aea4" },
    { "inactive_mid_1", "#dbd6d1" },
    { "inactive_text_color_2", "#929595" },
    { "inactive_color_2", "#f6f5f4" },
    { "inactive_hilight_2", "#ffffff" },
    { "inactive_shadow_2", "#b1aba5" },
    { "inactive_mid_2", "#d8d5d2" },
};

static const char *style_value(const ui_style *style, int index);
static void resolve_references(color_symbol_values *values);
static int symbol_index(const char *name);
static void set_value(color_symbol_values *values, int index, const char *value);

const char *appearance_color_symbol_name(int index)
{
    if (index < 0 || index >= APPEARANCE_COLOR_SYMBOL_COUNT) {
        return NULL;
    }
    return symbol_table[index].name;
}

const char *appearance_color_symbol_fallback(int index)
{
    return symbol_table[index].fallback;
}

void appearance_color_symbols_resolve(color_symbol_values *values, const ui_style *style,
                                      const themerc *rc)
{
    for (int i = 0; i < APPEARANCE_COLOR_SYMBOL_COUNT; i++) {
        const char *name = symbol_table[i].name;
        const char *theme_value = rc != NULL ? appearance_themerc_get(rc, name) : NULL;
        set_value(values, i, theme_value != NULL ? theme_value : style_value(style, i));
        values->items[i] = (xpm_symbol) { name, values->value[i] };
    }
    resolve_references(values);
}

xpm_symbols appearance_color_symbols_view(const color_symbol_values *values)
{
    return (xpm_symbols) { values->items, APPEARANCE_COLOR_SYMBOL_COUNT };
}

static const char *style_value(const ui_style *style, int index)
{
    if (style == NULL || style->color_symbols[index][0] == '\0') {
        return symbol_table[index].fallback;
    }
    return style->color_symbols[index];
}

static void resolve_references(color_symbol_values *values)
{
    for (int i = 0; i < APPEARANCE_COLOR_SYMBOL_COUNT; i++) {
        int target = symbol_index(values->value[i]);
        if (target >= 0) {
            set_value(values, i, values->value[target]);
        }
    }
}

static int symbol_index(const char *name)
{
    for (int i = 0; i < APPEARANCE_COLOR_SYMBOL_COUNT; i++) {
        if (g_ascii_strcasecmp(name, symbol_table[i].name) == 0) {
            return i;
        }
    }
    return -1;
}

static void set_value(color_symbol_values *values, int index, const char *value)
{
    if (value != values->value[index]) {
        g_strlcpy(values->value[index], value, COLOR_SYMBOL_VALUE_MAX);
    }
}
