#include "ui_text.h"

static const double half_pixel = 0.5;

static void underline_char(PangoLayout *layout, const char *text, int at);

void appearance_ui_text_draw(cairo_t *cr, const ui_text *ui, const text_box *box)
{
    PangoLayout *layout = appearance_text_layout(ui->font, box->text);

    pango_layout_set_width(layout, box->rect.width * PANGO_SCALE);
    pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
    pango_layout_set_alignment(layout, box->alignment);
    pango_layout_set_single_paragraph_mode(layout, TRUE);
    if (box->underline && box->underline_at >= 0) {
        underline_char(layout, box->text, box->underline_at);
    }
    int height = appearance_text_layout_size(layout).height;
    cairo_save(cr);
    cairo_rectangle(cr, box->rect.x, box->rect.y, box->rect.width, box->rect.height);
    cairo_clip(cr);
    cairo_move_to(cr, box->rect.x, box->rect.y + (box->rect.height - height) / 2);
    appearance_text_set_color(cr, box->color);
    pango_cairo_show_layout(cr, layout);
    cairo_restore(cr);
    g_object_unref(layout);
}

void appearance_ui_fill_rect(cairo_t *cr, const wm_rect *rect, const ui_color *color)
{
    cairo_rectangle(cr, rect->x, rect->y, rect->width, rect->height);
    appearance_text_set_color(cr, color);
    cairo_fill(cr);
}

void appearance_ui_panel(cairo_t *cr, const ui_style *style, wm_size size)
{
    wm_rect outer = { 0, 0, size.width, size.height };

    appearance_ui_fill_rect(cr, &outer, &style->background);
    cairo_rectangle(cr, half_pixel, half_pixel, size.width - PANEL_BORDER,
                    size.height - PANEL_BORDER);
    cairo_set_line_width(cr, PANEL_BORDER);
    appearance_text_set_color(cr, &style->border);
    cairo_stroke(cr);
}

static void underline_char(PangoLayout *layout, const char *text, int at)
{
    PangoAttrList *attributes = pango_attr_list_new();
    PangoAttribute *underline = pango_attr_underline_new(PANGO_UNDERLINE_LOW);

    underline->start_index = (guint) at;
    underline->end_index = (guint) (g_utf8_next_char(text + at) - text);
    pango_attr_list_insert(attributes, underline);
    pango_layout_set_attributes(layout, attributes);
    pango_attr_list_unref(attributes);
}
