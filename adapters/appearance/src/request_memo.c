#include "request_memo.h"

#include <glib.h>

static ui_style style_of(const theme_request *request);
static bool styles_equal(const ui_style *a, const ui_style *b);
static bool symbols_equal(const ui_style *a, const ui_style *b);
static bool colors_equal(const ui_style *a, const ui_style *b);
static bool color_equal(const ui_color *a, const ui_color *b);

void appearance_request_memo_store(request_memo *memo, const theme_request *request)
{
    appearance_request_memo_clear(memo);
    *memo = (request_memo) {
        .is_set = true,
        .name = g_strdup(request->name),
        .title_font = g_strdup(request->title_font),
        .max_output_scale = request->max_output_scale,
        .style = style_of(request),
    };
}

void appearance_request_memo_clear(request_memo *memo)
{
    g_free(memo->name);
    g_free(memo->title_font);
    *memo = (request_memo) { .is_set = false };
}

bool appearance_request_memo_matches(const request_memo *memo, const theme_request *request)
{
    ui_style style;

    if (!memo->is_set || g_strcmp0(memo->name, request->name) != 0
        || g_strcmp0(memo->title_font, request->title_font) != 0
        || memo->max_output_scale != request->max_output_scale) {
        return false;
    }
    style = style_of(request);
    return styles_equal(&memo->style, &style);
}

static ui_style style_of(const theme_request *request)
{
    return request->style != NULL ? *request->style : appearance_fallback_style();
}

static bool styles_equal(const ui_style *a, const ui_style *b)
{
    return symbols_equal(a, b) && colors_equal(a, b) && g_strcmp0(a->font, b->font) == 0
           && a->font_dpi == b->font_dpi;
}

static bool symbols_equal(const ui_style *a, const ui_style *b)
{
    for (int i = 0; i < APPEARANCE_COLOR_SYMBOL_COUNT; i++) {
        if (g_strcmp0(a->color_symbols[i], b->color_symbols[i]) != 0) {
            return false;
        }
    }
    return true;
}

static bool colors_equal(const ui_style *a, const ui_style *b)
{
    return color_equal(&a->background, &b->background)
           && color_equal(&a->foreground, &b->foreground)
           && color_equal(&a->selected_background, &b->selected_background)
           && color_equal(&a->selected_foreground, &b->selected_foreground)
           && color_equal(&a->border, &b->border)
           && color_equal(&a->insensitive_foreground, &b->insensitive_foreground);
}

static bool color_equal(const ui_color *a, const ui_color *b)
{
    return a->red == b->red && a->green == b->green && a->blue == b->blue && a->alpha == b->alpha;
}
