#include "label_painter.h"

#include "canvas.h"
#include "menu_layout.h"

enum {
    LABEL_INSET = MENU_BORDER + 3,
};

wm_size appearance_label_layout(const text_font *font, const char *text)
{
    PangoLayout *layout = appearance_text_layout(font, text);
    wm_size size = appearance_text_layout_size(layout);

    g_object_unref(layout);
    return (wm_size) { size.width + 2 * LABEL_INSET, size.height + 2 * LABEL_INSET };
}

cairo_surface_t *appearance_label_paint(const ui_text *ui, const label_paint_request *request)
{
    wm_size size = request->size;
    canvas canvas = appearance_canvas_create(size, request->scale);
    text_box box = {
        .text = request->text,
        .rect = { LABEL_INSET, LABEL_INSET, size.width - 2 * LABEL_INSET,
                  size.height - 2 * LABEL_INSET },
        .color = &ui->style->foreground,
        .alignment = PANGO_ALIGN_CENTER,
    };

    appearance_ui_panel(canvas.cr, ui->style, size, MENU_BORDER);
    appearance_ui_text_draw(canvas.cr, ui, &box);
    return appearance_canvas_finish(&canvas);
}
