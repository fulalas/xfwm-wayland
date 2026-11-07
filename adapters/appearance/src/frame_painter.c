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

typedef struct frame_job {
    cairo_t *cr;
    const frame_painter *painter;
    const frame_paint_request *request;
    device_map map;
} frame_job;

typedef struct pattern_axis {
    int pixels;
    int box_start;
    int box_length;
    int origin;
    double zoom;
    bool anchored_end;
    bool stretched;
    bool plain;
} pattern_axis;

typedef struct pixmap_op {
    const wm_paint_op *op;
    const op_source *source;
    op_anchor anchor;
} pixmap_op;

static device_map map_for(const frame_paint_request *request);
static int device_x(const device_map *map, int x);
static int device_y(const device_map *map, int y);
static wm_rect device_rect(const device_map *map, const wm_rect *rect);
static void set_logical_matrix(cairo_t *cr, const device_map *map);
static void paint_ops(const frame_job *job);
static void paint_op(const frame_job *job, const wm_paint_op *op);
static void paint_title(const frame_job *job);
static bool op_is_in_band(const wm_paint_op *op, const wm_rect *band);
static bool title_is_in_band(const frame_painter *painter, const frame_paint_request *request);
static op_source source_for(const frame_painter *painter, const frame_paint_request *request,
                            wm_pixmap_ref ref);
static void paint_pixmap(const frame_job *job, const wm_paint_op *op, const op_source *source);
static wm_rect visible_area(const frame_job *job, const pixmap_op *pixmap);
static void paint_icon(const frame_job *job, const wm_paint_op *op, cairo_surface_t *icon);
static wm_rect image_rect(const pixmap_op *pixmap);
static cairo_pattern_t *create_pattern(const pixmap_op *pixmap, const device_map *map);
static pattern_axis horizontal_axis(const pixmap_op *pixmap, const device_map *map,
                                    const wm_rect *box);
static pattern_axis vertical_axis(const pixmap_op *pixmap, const device_map *map,
                                  const wm_rect *box);
static double axis_scale(const pattern_axis *axis);
static double axis_start(const pattern_axis *axis);
static cairo_extend_t extend_for(wm_fill_mode mode);
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
    frame_job job = { cairo_create(surface), painter, request, map };

    if (request->layout != NULL && request->layout->has_frame) {
        paint_ops(&job);
    }
    cairo_destroy(job.cr);
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

static void paint_ops(const frame_job *job)
{
    const wm_decor_layout *layout = job->request->layout;

    for (size_t i = 0; i < layout->op_count && i < WM_DECOR_OP_MAX; i++) {
        paint_op(job, &layout->ops[i]);
    }
    if (title_is_in_band(job->painter, job->request)) {
        paint_title(job);
    }
}

static void paint_op(const frame_job *job, const wm_paint_op *op)
{
    if (!op_is_in_band(op, &job->request->band)) {
        return;
    }
    op_source source = source_for(job->painter, job->request, op->pixmap);
    if (source.surface == NULL) {
        return;
    }
    if (op->pixmap.kind == WM_PIXMAP_APP_ICON) {
        paint_icon(job, op, source.surface);
    } else {
        paint_pixmap(job, op, &source);
    }
}

static void paint_title(const frame_job *job)
{
    cairo_save(job->cr);
    set_logical_matrix(job->cr, &job->map);
    appearance_title_draw(job->cr, job->painter->title, job->request);
    cairo_restore(job->cr);
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

static void paint_pixmap(const frame_job *job, const wm_paint_op *op, const op_source *source)
{
    pixmap_op pixmap = { op, source, anchor_of(op->pixmap) };
    wm_rect visible = visible_area(job, &pixmap);
    wm_rect area = device_rect(&job->map, &visible);
    if (wm_rect_is_empty(&visible) || wm_rect_is_empty(&area)) {
        return;
    }
    cairo_pattern_t *pattern = create_pattern(&pixmap, &job->map);
    cairo_save(job->cr);
    cairo_set_antialias(job->cr, CAIRO_ANTIALIAS_NONE);
    cairo_rectangle(job->cr, area.x, area.y, area.width, area.height);
    cairo_set_source(job->cr, pattern);
    cairo_fill(job->cr);
    cairo_restore(job->cr);
    cairo_pattern_destroy(pattern);
}

static wm_rect visible_area(const frame_job *job, const pixmap_op *pixmap)
{
    const wm_paint_op *op = pixmap->op;
    wm_rect visible = wm_rect_intersection(&op->dest, &op->clip);

    visible = wm_rect_intersection(&visible, &job->request->band);
    if (op->mode == WM_FILL_PLAIN) {
        wm_rect image = image_rect(pixmap);
        visible = wm_rect_intersection(&visible, &image);
    }
    return visible;
}

static void paint_icon(const frame_job *job, const wm_paint_op *op, cairo_surface_t *icon)
{
    wm_rect area = wm_rect_intersection(&op->dest, &op->clip);
    canvas_image image = { icon, op->dest, opaque_alpha };

    if (wm_rect_is_empty(&area)) {
        return;
    }
    cairo_save(job->cr);
    set_logical_matrix(job->cr, &job->map);
    cairo_rectangle(job->cr, area.x, area.y, area.width, area.height);
    cairo_clip(job->cr);
    appearance_canvas_paint_fitted(job->cr, &image);
    cairo_restore(job->cr);
}

static wm_rect image_rect(const pixmap_op *pixmap)
{
    const wm_rect *dest = &pixmap->op->dest;
    int width = pixmap->source->logical.width;
    int height = pixmap->source->logical.height;

    return (wm_rect) {
        pixmap->anchor.right ? dest->x + dest->width - width : dest->x,
        pixmap->anchor.bottom ? dest->y + dest->height - height : dest->y,
        width,
        height,
    };
}

static cairo_pattern_t *create_pattern(const pixmap_op *pixmap, const device_map *map)
{
    wm_rect box = device_rect(map, &pixmap->op->dest);
    pattern_axis x = horizontal_axis(pixmap, map, &box);
    pattern_axis y = vertical_axis(pixmap, map, &box);
    cairo_pattern_t *pattern = cairo_pattern_create_for_surface(pixmap->source->surface);
    cairo_matrix_t matrix;

    cairo_matrix_init_scale(&matrix, axis_scale(&x), axis_scale(&y));
    cairo_matrix_translate(&matrix, -axis_start(&x), -axis_start(&y));
    cairo_pattern_set_matrix(pattern, &matrix);
    cairo_pattern_set_extend(pattern, extend_for(pixmap->op->mode));
    cairo_pattern_set_filter(pattern, CAIRO_FILTER_NEAREST);
    return pattern;
}

static pattern_axis horizontal_axis(const pixmap_op *pixmap, const device_map *map,
                                    const wm_rect *box)
{
    const wm_paint_op *op = pixmap->op;

    return (pattern_axis) {
        .pixels = pixmap->source->pixels.width,
        .box_start = box->x,
        .box_length = box->width,
        .origin = device_x(map, op->origin.x),
        .zoom = pixmap->source->zoom,
        .anchored_end = pixmap->anchor.right,
        .stretched = op->mode == WM_FILL_STRETCH_HORIZONTAL,
        .plain = op->mode == WM_FILL_PLAIN,
    };
}

static pattern_axis vertical_axis(const pixmap_op *pixmap, const device_map *map,
                                  const wm_rect *box)
{
    const wm_paint_op *op = pixmap->op;

    return (pattern_axis) {
        .pixels = pixmap->source->pixels.height,
        .box_start = box->y,
        .box_length = box->height,
        .origin = device_y(map, op->origin.y),
        .zoom = pixmap->source->zoom,
        .anchored_end = pixmap->anchor.bottom,
        .stretched = op->mode == WM_FILL_STRETCH_VERTICAL,
        .plain = op->mode == WM_FILL_PLAIN,
    };
}

static double axis_scale(const pattern_axis *axis)
{
    if (axis->stretched && axis->box_length > 0) {
        return (double) axis->pixels / axis->box_length;
    }
    return 1.0 / axis->zoom;
}

static double axis_start(const pattern_axis *axis)
{
    int length = (int) lround(axis->pixels * axis->zoom);

    if (axis->stretched && axis->box_length > 0) {
        return axis->origin;
    }
    if (axis->anchored_end) {
        return axis->box_start + axis->box_length - length;
    }
    return axis->plain ? axis->box_start : axis->origin;
}

static cairo_extend_t extend_for(wm_fill_mode mode)
{
    if (mode == WM_FILL_TILE) {
        return CAIRO_EXTEND_REPEAT;
    }
    return mode == WM_FILL_PLAIN ? CAIRO_EXTEND_PAD : CAIRO_EXTEND_NONE;
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
