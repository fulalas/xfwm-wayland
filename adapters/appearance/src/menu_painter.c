#include "menu_painter.h"

#include "canvas.h"
#include "menu_labels.h"
#include "menu_layout.h"
#include "menu_marks.h"

typedef struct menu_paint {
    const ui_text *ui;
    const wm_menu_level *level;
    const menu_layout *layout;
    menu_columns columns;
} menu_paint;

static void paint_item(cairo_t *cr, const menu_paint *paint, size_t index);
static void paint_separator(cairo_t *cr, const menu_paint *paint, size_t index);
static const ui_color *item_color(const menu_paint *paint, size_t index);
static void paint_decorations(cairo_t *cr, const menu_paint *paint, size_t index);
static bool is_hovered(const wm_menu_level *level, size_t index);

cairo_surface_t *appearance_menu_paint(const ui_text *ui, const menu_paint_request *request)
{
    const menu_layout *layout = request->layout;
    menu_paint paint = { ui, request->level, layout, appearance_menu_columns(request->level) };
    canvas canvas = appearance_canvas_create(layout->size, request->scale);
    size_t count = MIN(layout->count, appearance_menu_item_count(request->level));

    appearance_ui_panel(canvas.cr, ui->style, layout->size, MENU_BORDER);
    for (size_t i = 0; i < count; i++) {
        paint_item(canvas.cr, &paint, i);
    }
    return appearance_canvas_finish(&canvas);
}

static void paint_item(cairo_t *cr, const menu_paint *paint, size_t index)
{
    const wm_rect *item = &paint->layout->items[index];

    if (appearance_menu_is_separator(paint->level, index)) {
        paint_separator(cr, paint, index);
        return;
    }
    if (is_hovered(paint->level, index)) {
        appearance_ui_fill_rect(cr, item, &paint->ui->style->selected_background);
    }
    int mnemonic_at;
    char *text = appearance_menu_item_text_with_mnemonic(paint->level, index, &mnemonic_at);
    text_box box = {
        .text = text,
        .rect = appearance_menu_label_rect(&paint->columns, item),
        .color = item_color(paint, index),
        .alignment = PANGO_ALIGN_LEFT,
        .underline = paint->level->mnemonics,
        .underline_at = mnemonic_at,
    };
    appearance_ui_text_draw(cr, paint->ui, &box);
    g_free(text);
    paint_decorations(cr, paint, index);
}

static void paint_separator(cairo_t *cr, const menu_paint *paint, size_t index)
{
    const wm_rect *item = &paint->layout->items[index];
    wm_rect line = { item->x, item->y + item->height / 2, item->width, 1 };

    appearance_ui_fill_rect(cr, &line, &paint->ui->style->border);
}

static const ui_color *item_color(const menu_paint *paint, size_t index)
{
    const ui_style *style = paint->ui->style;

    if (!paint->level->items[index].sensitive) {
        return &style->insensitive_foreground;
    }
    return is_hovered(paint->level, index) ? &style->selected_foreground : &style->foreground;
}

static void paint_decorations(cairo_t *cr, const menu_paint *paint, size_t index)
{
    const wm_menu_item *item = &paint->level->items[index];
    const wm_rect *row = &paint->layout->items[index];
    int top = row->y + (row->height - MENU_TOGGLE_SIZE) / 2;
    wm_rect toggle = { row->x + MENU_ITEM_PADDING_X, top, MENU_TOGGLE_SIZE, MENU_TOGGLE_SIZE };
    wm_rect arrow = { row->x + row->width - MENU_ITEM_PADDING_X - MENU_ARROW_SIZE, top,
                      MENU_ARROW_SIZE, MENU_ARROW_SIZE };

    appearance_text_set_color(cr, item_color(paint, index));
    appearance_menu_mark_draw(cr, item->mark, &toggle);
    if (item->has_submenu) {
        appearance_menu_arrow_draw(cr, &arrow);
    }
}

static bool is_hovered(const wm_menu_level *level, size_t index)
{
    return level->selected == (int) index && level->items[index].sensitive;
}
