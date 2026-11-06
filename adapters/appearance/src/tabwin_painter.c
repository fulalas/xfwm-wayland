#include "tabwin_painter.h"

#include "canvas.h"
#include "tabwin_layout.h"

enum {
    SELECTED_RADIUS = 5,
};

static const double minimized_icon_alpha = 0.55;
static const double opaque_alpha = 1.0;
static const double half_pixel = 0.5;

typedef struct tabwin_paint {
    const ui_text *ui;
    const cycle_paint_request *request;
    int label_height;
} tabwin_paint;

static void paint_background(cairo_t *cr, const ui_style *style, wm_size size);
static void paint_cell(cairo_t *cr, const tabwin_paint *paint, size_t index);
static void paint_highlight(cairo_t *cr, const tabwin_paint *paint, size_t index);
static void paint_row_text(cairo_t *cr, const tabwin_paint *paint, size_t index);
static void paint_label(cairo_t *cr, const tabwin_paint *paint);
static cairo_surface_t *icon_of(const cycle_paint_request *request, size_t index);

cairo_surface_t *appearance_tabwin_paint(const ui_text *ui, const cycle_paint_request *request)
{
    const cycle_layout *layout = request->layout;
    tabwin_paint paint = { ui, request, appearance_text_line_height(ui->font) };
    canvas canvas = appearance_canvas_create(layout->size, request->scale);
    size_t count = MIN(layout->count, appearance_tabwin_item_count(request->view));

    paint_background(canvas.cr, ui->style, layout->size);
    for (size_t i = 0; i < count; i++) {
        paint_cell(canvas.cr, &paint, i);
    }
    if (request->view->mode != WM_TABWIN_LIST) {
        paint_label(canvas.cr, &paint);
    }
    return appearance_canvas_finish(&canvas);
}

static void paint_background(cairo_t *cr, const ui_style *style, wm_size size)
{
    wm_rect outer = { 0, 0, size.width, size.height };

    appearance_canvas_rounded_rect(cr, &outer, TABWIN_RADIUS);
    appearance_text_set_color(cr, &style->background);
    cairo_fill(cr);
    cairo_save(cr);
    cairo_translate(cr, half_pixel, half_pixel);
    outer.width -= TABWIN_BORDER;
    outer.height -= TABWIN_BORDER;
    appearance_canvas_rounded_rect(cr, &outer, TABWIN_RADIUS);
    cairo_set_line_width(cr, TABWIN_BORDER);
    appearance_text_set_color(cr, &style->selected_background);
    cairo_stroke(cr);
    cairo_restore(cr);
}

static void paint_cell(cairo_t *cr, const tabwin_paint *paint, size_t index)
{
    const wm_cycle_view *view = paint->request->view;
    canvas_image icon = {
        .surface = icon_of(paint->request, index),
        .rect = paint->request->layout->icons[index],
        .alpha = view->items[index].minimized ? minimized_icon_alpha : opaque_alpha,
    };

    if (index == view->selected) {
        paint_highlight(cr, paint, index);
    }
    appearance_canvas_paint_fitted(cr, &icon);
    if (view->mode == WM_TABWIN_LIST) {
        paint_row_text(cr, paint, index);
    }
}

static void paint_highlight(cairo_t *cr, const tabwin_paint *paint, size_t index)
{
    const wm_rect *cell = &paint->request->layout->items[index];

    appearance_canvas_rounded_rect(cr, cell, SELECTED_RADIUS);
    appearance_text_set_color(cr, &paint->ui->style->selected_background);
    cairo_fill(cr);
}

static void paint_row_text(cairo_t *cr, const tabwin_paint *paint, size_t index)
{
    const ui_style *style = paint->ui->style;
    const wm_cycle_view *view = paint->request->view;
    const wm_rect *cell = &paint->request->layout->items[index];
    bool selected = index == view->selected;
    text_box box = {
        .text = view->items[index].title,
        .rect = appearance_tabwin_text_rect(view, cell, paint->label_height),
        .color = selected ? &style->selected_foreground : &style->foreground,
        .alignment = PANGO_ALIGN_LEFT,
    };

    appearance_ui_text_draw(cr, paint->ui, &box);
}

static void paint_label(cairo_t *cr, const tabwin_paint *paint)
{
    const wm_cycle_view *view = paint->request->view;

    if (view->selected >= appearance_tabwin_item_count(view)) {
        return;
    }
    text_box box = {
        .text = view->items[view->selected].title,
        .rect = appearance_tabwin_label_rect(paint->request->layout->size, paint->label_height),
        .color = &paint->ui->style->foreground,
        .alignment = PANGO_ALIGN_CENTER,
    };
    appearance_ui_text_draw(cr, paint->ui, &box);
}

static cairo_surface_t *icon_of(const cycle_paint_request *request, size_t index)
{
    return request->icons != NULL ? request->icons[index] : NULL;
}
