#include "frame_painter.h"

#include <math.h>

#include "canvas.h"

static const double opaque_alpha = 1.0;

typedef struct device_map {
    double scale;
    double origin_x;
    double origin_y;
    int base_x;
    int base_y;
} device_map;

typedef struct op_source {
    cairo_surface_t *surface;
    double zoom;
    wm_size logical;
    wm_size pixels;
} op_source;

typedef struct op_anchor {
    bool right;
    bool bottom;
} op_anchor;

static device_map map_for(const frame_paint_request *request);
static int device_x(const device_map *map, int x);
static int device_y(const device_map *map, int y);
static wm_rect device_rect(const device_map *map, const wm_rect *rect);
static void set_logical_matrix(cairo_t *cr, const device_map *map);
static void paint_ops(cairo_t *cr, const frame_painter *painter,
                      const frame_paint_request *request, const device_map *map);
static bool op_is_in_band(const wm_paint_op *op, const wm_rect *band);
static bool title_is_in_band(const frame_painter *painter, const frame_paint_request *request);
static op_source source_for(const frame_painter *painter, const frame_paint_request *request,
                            wm_pixmap_ref ref);
static void paint_pixmap(cairo_t *cr, const wm_paint_op *op, const op_source *source,
                         const frame_paint_request *request, const device_map *map);
static void paint_icon(cairo_t *cr, const wm_paint_op *op, cairo_surface_t *icon,
                       const device_map *map);
static wm_rect image_rect(const wm_paint_op *op, const op_source *source, op_anchor anchor);
static cairo_pattern_t *create_pattern(const wm_paint_op *op, const op_source *source,
                                       const device_map *map, op_anchor anchor);
static op_anchor anchor_of(wm_pixmap_ref ref);

cairo_surface_t *appearance_frame_paint(const frame_painter *painter,
                                        const frame_paint_request *request)
{
    device_map map = map_for(request);
    int width = device_x(&map, request->band.x + request->band.width);
    int height = device_y(&map, request->band.y + request->band.height);
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                                          width > 0 ? width : 1,
                                                          height > 0 ? height : 1);
    cairo_t *cr = cairo_create(surface);

    if (request->layout != NULL && request->layout->has_frame) {
        paint_ops(cr, painter, request, &map);
    }
    cairo_destroy(cr);
    cairo_surface_flush(surface);
    return surface;
}

static device_map map_for(const frame_paint_request *request)
{
    double scale = request->scale > 0.0 ? request->scale : 1.0;
    double origin_x = request->origin.x;
    double origin_y = request->origin.y;

    return (device_map) {
        .scale = scale,
        .origin_x = origin_x,
        .origin_y = origin_y,
        .base_x = (int) round((origin_x + request->band.x) * scale),
        .base_y = (int) round((origin_y + request->band.y) * scale),
    };
}

static int device_x(const device_map *map, int x)
{
    return (int) round((map->origin_x + x) * map->scale) - map->base_x;
}

static int device_y(const device_map *map, int y)
{
    return (int) round((map->origin_y + y) * map->scale) - map->base_y;
}

static wm_rect device_rect(const device_map *map, const wm_rect *rect)
{
    int left = device_x(map, rect->x);
    int top = device_y(map, rect->y);

    return (wm_rect) { left, top, device_x(map, rect->x + rect->width) - left,
                       device_y(map, rect->y + rect->height) - top };
}

static void set_logical_matrix(cairo_t *cr, const device_map *map)
{
    cairo_matrix_t matrix;

    cairo_matrix_init(&matrix, map->scale, 0.0, 0.0, map->scale,
                      map->origin_x * map->scale - map->base_x,
                      map->origin_y * map->scale - map->base_y);
    cairo_set_matrix(cr, &matrix);
}

static void paint_ops(cairo_t *cr, const frame_painter *painter,
                      const frame_paint_request *request, const device_map *map)
{
    const wm_decor_layout *layout = request->layout;

    for (size_t i = 0; i < layout->op_count && i < WM_DECOR_OP_MAX; i++) {
        const wm_paint_op *op = &layout->ops[i];
        if (!op_is_in_band(op, &request->band)) {
            continue;
        }
        op_source source = source_for(painter, request, op->pixmap);
        if (source.surface == NULL) {
            continue;
        }
        if (op->pixmap.kind == WM_PIXMAP_APP_ICON) {
            paint_icon(cr, op, source.surface, map);
        } else {
            paint_pixmap(cr, op, &source, request, map);
        }
    }
    if (title_is_in_band(painter, request)) {
        cairo_save(cr);
        set_logical_matrix(cr, map);
        appearance_title_draw(cr, painter->title, request);
        cairo_restore(cr);
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
        return (op_source) { .surface = request->icon, .zoom = 1.0 };
    }
    if (painter->theme == NULL) {
        return (op_source) { .surface = NULL, .zoom = 1.0 };
    }
    cairo_surface_t *surface = appearance_theme_pixmap(painter->theme, ref);
    double pixmap_scale = appearance_theme_pixmap_scale(painter->theme);
    if (surface == NULL || pixmap_scale <= 0.0) {
        return (op_source) { .surface = NULL, .zoom = 1.0 };
    }
    double request_scale = request->scale > 0.0 ? request->scale : 1.0;
    return (op_source) {
        .surface = surface,
        .zoom = request_scale / pixmap_scale,
        .logical = appearance_surface_logical_size(surface, pixmap_scale),
        .pixels = appearance_surface_size(surface),
    };
}

static void paint_pixmap(cairo_t *cr, const wm_paint_op *op, const op_source *source,
                         const frame_paint_request *request, const device_map *map)
{
    op_anchor anchor = anchor_of(op->pixmap);
    wm_rect visible = wm_rect_intersection(&op->dest, &op->clip);
    visible = wm_rect_intersection(&visible, &request->band);
    if (op->mode == WM_FILL_PLAIN) {
        wm_rect image = image_rect(op, source, anchor);
        visible = wm_rect_intersection(&visible, &image);
    }
    wm_rect area = device_rect(map, &visible);
    if (wm_rect_is_empty(&visible) || wm_rect_is_empty(&area)) {
        return;
    }
    cairo_pattern_t *pattern = create_pattern(op, source, map, anchor);
    cairo_save(cr);
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
    cairo_rectangle(cr, area.x, area.y, area.width, area.height);
    cairo_set_source(cr, pattern);
    cairo_fill(cr);
    cairo_restore(cr);
    cairo_pattern_destroy(pattern);
}

static void paint_icon(cairo_t *cr, const wm_paint_op *op, cairo_surface_t *icon,
                       const device_map *map)
{
    wm_rect area = wm_rect_intersection(&op->dest, &op->clip);
    canvas_image image = { icon, op->dest, opaque_alpha };

    if (wm_rect_is_empty(&area)) {
        return;
    }
    cairo_save(cr);
    set_logical_matrix(cr, map);
    cairo_rectangle(cr, area.x, area.y, area.width, area.height);
    cairo_clip(cr);
    appearance_canvas_paint_fitted(cr, &image);
    cairo_restore(cr);
}

static wm_rect image_rect(const wm_paint_op *op, const op_source *source, op_anchor anchor)
{
    int width = source->logical.width;
    int height = source->logical.height;

    return (wm_rect) {
        anchor.right ? op->dest.x + op->dest.width - width : op->dest.x,
        anchor.bottom ? op->dest.y + op->dest.height - height : op->dest.y,
        width,
        height,
    };
}

static cairo_pattern_t *create_pattern(const wm_paint_op *op, const op_source *source,
                                       const device_map *map, op_anchor anchor)
{
    wm_rect box = device_rect(map, &op->dest);
    double zoom = source->zoom;
    int width = (int) lround(source->pixels.width * zoom);
    int height = (int) lround(source->pixels.height * zoom);
    double scale_x = 1.0 / zoom;
    double scale_y = 1.0 / zoom;
    double start_x = anchor.right ? box.x + box.width - width : device_x(map, op->origin.x);
    double start_y = anchor.bottom ? box.y + box.height - height : device_y(map, op->origin.y);
    cairo_extend_t extend = op->mode == WM_FILL_TILE    ? CAIRO_EXTEND_REPEAT
                            : op->mode == WM_FILL_PLAIN ? CAIRO_EXTEND_PAD
                                                        : CAIRO_EXTEND_NONE;

    if (op->mode == WM_FILL_PLAIN) {
        start_x = anchor.right ? box.x + box.width - width : box.x;
        start_y = anchor.bottom ? box.y + box.height - height : box.y;
    }
    if (op->mode == WM_FILL_STRETCH_HORIZONTAL && box.width > 0) {
        scale_x = (double) source->pixels.width / box.width;
        start_x = device_x(map, op->origin.x);
    }
    if (op->mode == WM_FILL_STRETCH_VERTICAL && box.height > 0) {
        scale_y = (double) source->pixels.height / box.height;
        start_y = device_y(map, op->origin.y);
    }
    cairo_pattern_t *pattern = cairo_pattern_create_for_surface(source->surface);
    cairo_matrix_t matrix;
    cairo_matrix_init_scale(&matrix, scale_x, scale_y);
    cairo_matrix_translate(&matrix, -start_x, -start_y);
    cairo_pattern_set_matrix(pattern, &matrix);
    cairo_pattern_set_extend(pattern, extend);
    cairo_pattern_set_filter(pattern, CAIRO_FILTER_NEAREST);
    return pattern;
}

static op_anchor anchor_of(wm_pixmap_ref ref)
{
    bool corner = ref.kind == WM_PIXMAP_CORNER || ref.kind == WM_PIXMAP_CORNER_STRETCH;
    bool side = ref.kind == WM_PIXMAP_SIDE || ref.kind == WM_PIXMAP_SIDE_STRETCH;
    bool right_corner = ref.index == WM_CORNER_TOP_RIGHT || ref.index == WM_CORNER_BOTTOM_RIGHT;
    bool bottom_corner = ref.index == WM_CORNER_BOTTOM_LEFT || ref.index == WM_CORNER_BOTTOM_RIGHT;

    return (op_anchor) {
        .right = (corner && right_corner) || (side && ref.index == WM_SIDE_RIGHT),
        .bottom = (corner && bottom_corner) || (side && ref.index == WM_SIDE_BOTTOM),
    };
}
