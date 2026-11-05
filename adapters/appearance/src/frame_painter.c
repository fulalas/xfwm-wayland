#include "frame_painter.h"

#include "canvas.h"

static const double opaque_alpha = 1.0;

typedef struct op_source {
    cairo_surface_t *surface;
    double scale;
} op_source;

static void paint_ops(cairo_t *cr, const frame_painter *painter,
                      const frame_paint_request *request);
static bool op_is_in_band(const wm_paint_op *op, const wm_rect *band);
static bool title_is_in_band(const frame_painter *painter, const frame_paint_request *request);
static op_source source_for(const frame_painter *painter, const frame_paint_request *request,
                            wm_pixmap_ref ref);
static void paint_op(cairo_t *cr, const wm_paint_op *op, op_source source, const wm_rect *band);
static void paint_pixmap(cairo_t *cr, const wm_paint_op *op, op_source source,
                         const wm_rect *band);
static void paint_icon(cairo_t *cr, const wm_paint_op *op, cairo_surface_t *icon);
static cairo_pattern_t *create_pattern(const wm_paint_op *op, op_source source);
static cairo_pattern_t *create_plain_pattern(const wm_paint_op *op, op_source source,
                                             const wm_rect *area);
static cairo_matrix_t pattern_matrix(const wm_paint_op *op, op_source source);
static wm_rect fill_area(const wm_paint_op *op, op_source source);

cairo_surface_t *appearance_frame_paint(const frame_painter *painter,
                                        const frame_paint_request *request)
{
    wm_size band = { request->band.width, request->band.height };
    canvas canvas = appearance_canvas_create(band, request->scale);

    cairo_translate(canvas.cr, -request->band.x, -request->band.y);
    cairo_rectangle(canvas.cr, request->band.x, request->band.y, band.width, band.height);
    cairo_clip(canvas.cr);
    if (request->layout != NULL && request->layout->has_frame) {
        paint_ops(canvas.cr, painter, request);
    }
    return appearance_canvas_finish(&canvas);
}

static void paint_ops(cairo_t *cr, const frame_painter *painter,
                      const frame_paint_request *request)
{
    const wm_decor_layout *layout = request->layout;

    for (size_t i = 0; i < layout->op_count && i < WM_DECOR_OP_MAX; i++) {
        const wm_paint_op *op = &layout->ops[i];
        if (op_is_in_band(op, &request->band)) {
            paint_op(cr, op, source_for(painter, request, op->pixmap), &request->band);
        }
    }
    if (title_is_in_band(painter, request)) {
        appearance_title_draw(cr, painter->title, request);
    }
}

static bool op_is_in_band(const wm_paint_op *op, const wm_rect *band)
{
    wm_rect visible = wm_rect_intersection(&op->dest, &op->clip);

    return wm_rect_intersects(&visible, band);
}

static bool title_is_in_band(const frame_painter *painter, const frame_paint_request *request)
{
    const wm_text_op *title = &request->layout->title_text;

    return title->visible && painter->title != NULL
           && wm_rect_intersects(&title->clip, &request->band);
}

static op_source source_for(const frame_painter *painter, const frame_paint_request *request,
                            wm_pixmap_ref ref)
{
    if (ref.kind == WM_PIXMAP_APP_ICON) {
        return (op_source) { request->icon, 1.0 };
    }
    if (painter->theme == NULL) {
        return (op_source) { NULL, 1.0 };
    }
    return (op_source) {
        appearance_theme_pixmap(painter->theme, ref),
        appearance_theme_pixmap_scale(painter->theme),
    };
}

static void paint_op(cairo_t *cr, const wm_paint_op *op, op_source source, const wm_rect *band)
{
    if (source.surface == NULL) {
        return;
    }
    if (op->pixmap.kind == WM_PIXMAP_APP_ICON) {
        paint_icon(cr, op, source.surface);
    } else {
        paint_pixmap(cr, op, source, band);
    }
}

static void paint_pixmap(cairo_t *cr, const wm_paint_op *op, op_source source,
                         const wm_rect *band)
{
    wm_rect fill = fill_area(op, source);
    wm_rect area = wm_rect_intersection(&fill, band);

    if (wm_rect_is_empty(&area)) {
        return;
    }
    cairo_pattern_t *pattern = op->mode == WM_FILL_PLAIN ? create_plain_pattern(op, source, &area)
                                                         : create_pattern(op, source);
    cairo_save(cr);
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
    cairo_rectangle(cr, area.x, area.y, area.width, area.height);
    cairo_set_source(cr, pattern);
    cairo_fill(cr);
    cairo_restore(cr);
    cairo_pattern_destroy(pattern);
}

static void paint_icon(cairo_t *cr, const wm_paint_op *op, cairo_surface_t *icon)
{
    wm_rect area = wm_rect_intersection(&op->dest, &op->clip);
    canvas_image image = { icon, op->dest, opaque_alpha };

    if (wm_rect_is_empty(&area)) {
        return;
    }
    cairo_save(cr);
    cairo_rectangle(cr, area.x, area.y, area.width, area.height);
    cairo_clip(cr);
    appearance_canvas_paint_fitted(cr, &image);
    cairo_restore(cr);
}

static cairo_pattern_t *create_pattern(const wm_paint_op *op, op_source source)
{
    cairo_pattern_t *pattern = cairo_pattern_create_for_surface(source.surface);
    cairo_matrix_t matrix = pattern_matrix(op, source);

    cairo_pattern_set_matrix(pattern, &matrix);
    switch (op->mode) {
    case WM_FILL_TILE:
        cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);
        break;
    default:
        cairo_pattern_set_extend(pattern, CAIRO_EXTEND_NONE);
        cairo_pattern_set_filter(pattern, CAIRO_FILTER_NEAREST);
        break;
    }
    return pattern;
}

static cairo_matrix_t pattern_matrix(const wm_paint_op *op, op_source source)
{
    wm_size pixels = appearance_surface_size(source.surface);
    double scale_x = source.scale;
    double scale_y = source.scale;
    cairo_matrix_t matrix;

    if (op->mode == WM_FILL_STRETCH_HORIZONTAL && op->dest.width > 0) {
        scale_x = (double) pixels.width / op->dest.width;
    }
    if (op->mode == WM_FILL_STRETCH_VERTICAL && op->dest.height > 0) {
        scale_y = (double) pixels.height / op->dest.height;
    }
    cairo_matrix_init_scale(&matrix, scale_x, scale_y);
    cairo_matrix_translate(&matrix, -op->origin.x, -op->origin.y);
    return matrix;
}

static cairo_pattern_t *create_plain_pattern(const wm_paint_op *op, op_source source,
                                             const wm_rect *area)
{
    double scale = source.scale;
    cairo_surface_t *part = cairo_surface_create_for_rectangle(
        source.surface, (area->x - op->dest.x) * scale, (area->y - op->dest.y) * scale,
        area->width * scale, area->height * scale);
    cairo_pattern_t *pattern = cairo_pattern_create_for_surface(part);
    cairo_matrix_t matrix;

    cairo_surface_destroy(part);
    cairo_matrix_init_scale(&matrix, scale, scale);
    cairo_matrix_translate(&matrix, -area->x, -area->y);
    cairo_pattern_set_matrix(pattern, &matrix);
    cairo_pattern_set_extend(pattern, CAIRO_EXTEND_PAD);
    return pattern;
}

static wm_rect fill_area(const wm_paint_op *op, op_source source)
{
    wm_rect area = wm_rect_intersection(&op->dest, &op->clip);

    if (op->mode != WM_FILL_PLAIN) {
        return area;
    }
    wm_size pixels = appearance_surface_size(source.surface);
    wm_rect image = {
        op->dest.x, op->dest.y, (int) (pixels.width / source.scale),
        (int) (pixels.height / source.scale),
    };
    return wm_rect_intersection(&area, &image);
}
