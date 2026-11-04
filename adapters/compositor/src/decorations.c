#include <stdlib.h>
#include <string.h>
#include <wlr/types/wlr_scene.h>

#include "cairo_buffer.h"
#include "decorations.h"
#include "frame_bands.h"

typedef struct band_mark {
    bool painted;
    wm_rect rect;
    double scale;
} band_mark;

struct decorations {
    const compositor_painter *painter;
    struct wlr_scene_tree *tree;
    struct wlr_scene_buffer *bands[FRAME_BAND_COUNT];
    band_mark marks[FRAME_BAND_COUNT];
    wm_decor_layout painted_layout;
    char *painted_title;
};

typedef struct band_job {
    frame_band band;
    wm_rect rect;
} band_job;

static bool band_needs_paint(const decorations *decorations, const decor_paint *paint,
                             const band_job *job);
static bool shows_icon(const wm_decor_layout *layout, const wm_rect *band);
static bool shows_title(const wm_decor_layout *layout, const wm_rect *band);
static bool title_changed(const decorations *decorations, const decor_paint *paint);
static void remember_title(decorations *decorations, const char *title);
static bool same_ops_in_band(const wm_decor_layout *a, const wm_decor_layout *b,
                             const wm_rect *band);
static size_t next_op_in_band(const wm_decor_layout *layout, const wm_rect *band, size_t start);
static size_t op_limit(const wm_decor_layout *layout);
static bool op_touches(const wm_paint_op *op, const wm_rect *band);
static void paint_band(decorations *decorations, const decor_paint *paint, const band_job *job);
static cairo_surface_t *render_band(const decorations *decorations, const decor_paint *paint,
                                    const wm_rect *rect);

decorations *decorations_create(struct wlr_scene_tree *parent, const compositor_painter *painter)
{
    decorations *d = calloc(1, sizeof(*d));
    if (d == NULL) {
        return NULL;
    }
    d->painter = painter;
    d->tree = wlr_scene_tree_create(parent);
    for (int i = 0; i < FRAME_BAND_COUNT && d->tree != NULL; i++) {
        d->bands[i] = wlr_scene_buffer_create(d->tree, NULL);
    }
    if (d->tree == NULL) {
        free(d);
        return NULL;
    }
    wlr_scene_node_set_enabled(&d->tree->node, false);
    return d;
}

void decorations_destroy(decorations *d)
{
    wlr_scene_node_destroy(&d->tree->node);
    free(d->painted_title);
    free(d);
}

void decorations_paint(decorations *d, const decor_paint *paint)
{
    if (!paint->layout->has_frame) {
        decorations_hide(d);
        return;
    }
    frame_bands bands = frame_bands_compute(paint->layout);
    for (int i = 0; i < FRAME_BAND_COUNT; i++) {
        band_job job = {i, bands.rects[i]};
        if (band_needs_paint(d, paint, &job)) {
            paint_band(d, paint, &job);
        }
    }
    d->painted_layout = *paint->layout;
    remember_title(d, paint->title);
    wlr_scene_node_set_enabled(&d->tree->node, true);
}

void decorations_invalidate(decorations *d)
{
    for (int i = 0; i < FRAME_BAND_COUNT; i++) {
        d->marks[i].painted = false;
    }
}

void decorations_hide(decorations *d)
{
    wlr_scene_node_set_enabled(&d->tree->node, false);
}

void decorations_set_opacity(decorations *d, float opacity)
{
    for (int i = 0; i < FRAME_BAND_COUNT; i++) {
        if (d->bands[i] != NULL) {
            wlr_scene_buffer_set_opacity(d->bands[i], opacity);
        }
    }
}

struct wlr_scene_tree *decorations_tree(const decorations *d)
{
    return d->tree;
}

static bool band_needs_paint(const decorations *d, const decor_paint *paint, const band_job *job)
{
    const band_mark *mark = &d->marks[job->band];
    if (!mark->painted || mark->scale != paint->scale || !wm_rect_equals(&mark->rect, &job->rect)) {
        return true;
    }
    if (shows_icon(paint->layout, &job->rect) || shows_icon(&d->painted_layout, &job->rect)) {
        return true;
    }
    if ((shows_title(paint->layout, &job->rect) || shows_title(&d->painted_layout, &job->rect))
        && title_changed(d, paint)) {
        return true;
    }
    return !same_ops_in_band(&d->painted_layout, paint->layout, &job->rect);
}

static bool shows_icon(const wm_decor_layout *layout, const wm_rect *band)
{
    for (size_t i = 0; i < op_limit(layout); i++) {
        const wm_paint_op *op = &layout->ops[i];
        if (op->pixmap.kind == WM_PIXMAP_APP_ICON && op_touches(op, band)) {
            return true;
        }
    }
    return false;
}

static bool shows_title(const wm_decor_layout *layout, const wm_rect *band)
{
    const wm_text_op *title = &layout->title_text;
    return title->visible && wm_rect_intersects(&title->clip, band);
}

static bool title_changed(const decorations *d, const decor_paint *paint)
{
    const char *title = paint->title != NULL ? paint->title : "";
    if (d->painted_title == NULL || strcmp(d->painted_title, title) != 0) {
        return true;
    }
    return memcmp(&d->painted_layout.title_text, &paint->layout->title_text,
                  sizeof(paint->layout->title_text)) != 0;
}

static void remember_title(decorations *d, const char *title)
{
    free(d->painted_title);
    d->painted_title = strdup(title != NULL ? title : "");
}

static bool same_ops_in_band(const wm_decor_layout *a, const wm_decor_layout *b,
                             const wm_rect *band)
{
    size_t i = next_op_in_band(a, band, 0);
    size_t j = next_op_in_band(b, band, 0);
    while (i < op_limit(a) && j < op_limit(b)) {
        if (memcmp(&a->ops[i], &b->ops[j], sizeof(a->ops[i])) != 0) {
            return false;
        }
        i = next_op_in_band(a, band, i + 1);
        j = next_op_in_band(b, band, j + 1);
    }
    return i == op_limit(a) && j == op_limit(b);
}

static size_t next_op_in_band(const wm_decor_layout *layout, const wm_rect *band, size_t start)
{
    size_t limit = op_limit(layout);
    for (size_t i = start; i < limit; i++) {
        if (op_touches(&layout->ops[i], band)) {
            return i;
        }
    }
    return limit;
}

static size_t op_limit(const wm_decor_layout *layout)
{
    return layout->op_count < WM_DECOR_OP_MAX ? layout->op_count : WM_DECOR_OP_MAX;
}

static bool op_touches(const wm_paint_op *op, const wm_rect *band)
{
    wm_rect area = wm_rect_intersection(&op->dest, &op->clip);
    return wm_rect_intersects(&area, band);
}

static void paint_band(decorations *d, const decor_paint *paint, const band_job *job)
{
    struct wlr_scene_buffer *node = d->bands[job->band];
    if (node == NULL) {
        return;
    }
    band_mark *mark = &d->marks[job->band];
    *mark = (band_mark){false, job->rect, paint->scale};
    if (wm_rect_is_empty(&job->rect)) {
        wlr_scene_node_set_enabled(&node->node, false);
        return;
    }
    cairo_surface_t *surface = render_band(d, paint, &job->rect);
    wm_size size = {job->rect.width, job->rect.height};
    mark->painted = cairo_buffer_attach_sized(node, surface, size);
    wlr_scene_node_set_position(&node->node, job->rect.x, job->rect.y);
    wlr_scene_node_set_enabled(&node->node, mark->painted);
}

static cairo_surface_t *render_band(const decorations *d, const decor_paint *paint,
                                    const wm_rect *rect)
{
    frame_paint_request request = {
        .layout = paint->layout,
        .band = *rect,
        .scale = paint->scale,
        .title = paint->title,
        .icon = paint->icon,
    };
    return d->painter->paint_frame(d->painter->self, &request);
}
