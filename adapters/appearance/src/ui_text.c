#include "ui_text.h"

void appearance_ui_text_draw(cairo_t *cr, const ui_text *ui, const text_box *box)
{
    PangoLayout *layout = appearance_text_layout(ui->font, box->text);

    pango_layout_set_width(layout, box->rect.width * PANGO_SCALE);
    pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
    pango_layout_set_alignment(layout, box->alignment);
    pango_layout_set_single_paragraph_mode(layout, TRUE);
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
