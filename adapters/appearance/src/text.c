#include "text.h"

static int measure_line_height(const text_font *font);

text_font appearance_text_font_create(const char *description)
{
    PangoFontMap *map = pango_cairo_font_map_get_default();
    text_font font = {
        .context = pango_font_map_create_context(map),
        .description = pango_font_description_from_string(description),
    };

    font.line_height = measure_line_height(&font);
    return font;
}

void appearance_text_font_destroy(text_font *font)
{
    g_clear_object(&font->context);
    g_clear_pointer(&font->description, pango_font_description_free);
}

PangoLayout *appearance_text_layout(const text_font *font, const char *text)
{
    PangoLayout *layout = pango_layout_new(font->context);

    pango_layout_set_font_description(layout, font->description);
    pango_layout_set_auto_dir(layout, FALSE);
    pango_layout_set_text(layout, text != NULL ? text : "", -1);
    return layout;
}

wm_size appearance_text_layout_size(PangoLayout *layout)
{
    PangoRectangle logical;

    pango_layout_get_pixel_extents(layout, NULL, &logical);
    return (wm_size) { logical.width, logical.height };
}

int appearance_text_line_height(const text_font *font)
{
    return font->line_height;
}

void appearance_text_set_color(cairo_t *cr, const ui_color *color)
{
    cairo_set_source_rgba(cr, color->red, color->green, color->blue, color->alpha);
}

static int measure_line_height(const text_font *font)
{
    PangoLayout *layout = appearance_text_layout(font, "");
    int height = appearance_text_layout_size(layout).height;

    g_object_unref(layout);
    return height;
}
