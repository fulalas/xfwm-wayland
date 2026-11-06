#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <wlr/types/wlr_buffer.h>
#include <wlr/types/wlr_scene.h>

#include "cairo_buffer.h"
#include "decorations.h"
#include "frame_bands.h"

typedef struct band_mark {
    bool painted;
    wm_rect rect;
    double scale;
    wm_point origin;
} band_mark;

typedef struct band_set {
    struct wlr_scene_tree *tree;
    struct wlr_scene_buffer *bands[FRAME_BAND_COUNT];
} band_set;

typedef struct band_paint {
    band_mark mark;
    struct wlr_buffer *buffer;
} band_paint;

struct decorations {
    const compositor_painter *painter;
    band_set main;
    band_set mirror;
    band_paint shown[FRAME_BAND_COUNT];
    band_paint spare[FRAME_BAND_COUNT];
    wm_decor_layout painted_layout;
    char *painted_title;
    cairo_surface_t *painted_icon;
};

typedef enum band_change {
    BAND_SAME,
    BAND_MOVED,
    BAND_CHANGED,
} band_change;

typedef struct band_job {
    frame_band band;
    wm_rect rect;
} band_job;

static band_change band_change_for(const decorations *decorations, const decor_paint *paint,
                                   const band_job *job);
static bool shows_icon(const wm_decor_layout *layout, const wm_rect *band);
static bool shows_title(const wm_decor_layout *layout, const wm_rect *band);
static bool title_changed(const decorations *decorations, const decor_paint *paint);
static bool same_phase(int a, int b, double scale);
static void remember_title(decorations *decorations, const char *title);
static void remember_icon(decorations *decorations, cairo_surface_t *icon);
static bool same_ops_in_band(const wm_decor_layout *a, const wm_decor_layout *b,
                             const wm_rect *band);
static size_t next_op_in_band(const wm_decor_layout *layout, const wm_rect *band, size_t start);
static size_t op_limit(const wm_decor_layout *layout);
static bool op_touches(const wm_paint_op *op, const wm_rect *band);
static void repaint_band(decorations *decorations, const decor_paint *paint, const band_job *job);
static void realign_band(decorations *decorations, const decor_paint *paint, const band_job *job);
static void attach_mirror(decorations *decorations, const decor_paint *paint, const band_job *job);
static bool spare_fits(const band_paint *spare, const decor_paint *paint, const band_job *job);
static band_paint render_paint(const decorations *decorations, const decor_paint *paint,
                               const wm_rect *rect, wm_point origin);
static void release_paint(band_paint *paint);
static void release_paints(decorations *decorations);
static bool band_set_create(band_set *set, struct wlr_scene_tree *parent);
static void band_set_destroy(band_set *set);
static void band_set_show(band_set *set, bool shown);
static void band_set_opacity(band_set *set, float opacity);
static void band_set_attach(band_set *set, frame_band band, struct wlr_buffer *buffer,
                            const wm_rect *rect);
static cairo_surface_t *render_band(const decorations *decorations, const decor_paint *paint,
                                    const wm_rect *rect, wm_point origin);

decorations *decorations_create(struct wlr_scene_tree *parent, const compositor_painter *painter)
{
    decorations *d = calloc(1, sizeof(*d));
    if (d == NULL) {
        return NULL;
    }
    d->painter = painter;
    if (!band_set_create(&d->main, parent)) {
        free(d);
        return NULL;
    }
    return d;
}

void decorations_destroy(decorations *d)
{
    band_set_destroy(&d->mirror);
    band_set_destroy(&d->main);
    release_paints(d);
    remember_icon(d, NULL);
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
        band_change change = band_change_for(d, paint, &job);
        if (change == BAND_CHANGED) {
            repaint_band(d, paint, &job);
        } else if (change == BAND_MOVED) {
            realign_band(d, paint, &job);
        }
    }
    d->painted_layout = *paint->layout;
    remember_title(d, paint->title);
    remember_icon(d, paint->icon);
    band_set_show(&d->main, true);
    band_set_show(&d->mirror, true);
}

void decorations_invalidate(decorations *d)
{
    for (int i = 0; i < FRAME_BAND_COUNT; i++) {
        d->shown[i].mark.painted = false;
    }
}

void decorations_hide(decorations *d)
{
    band_set_show(&d->main, false);
    band_set_show(&d->mirror, false);
}

void decorations_set_opacity(decorations *d, float opacity)
{
    band_set_opacity(&d->main, opacity);
}

struct wlr_scene_tree *decorations_mirror(decorations *d, struct wlr_scene_tree *parent)
{
    band_set_destroy(&d->mirror);
    if (parent == NULL || !band_set_create(&d->mirror, parent)) {
        return NULL;
    }
    decorations_invalidate(d);
    return d->mirror.tree;
}

struct wlr_scene_tree *decorations_tree(const decorations *d)
{
    return d->main.tree;
}

bool decorations_same_alignment(wm_point a, wm_point b, double scale)
{
    return same_phase(a.x, b.x, scale) && same_phase(a.y, b.y, scale);
}

static bool same_phase(int a, int b, double scale)
{
    if (scale == floor(scale)) {
        return true;
    }
    if (a < 0 || b < 0) {
        return a == b;
    }
    double device_a = a * scale;
    double device_b = b * scale;
    return device_a - floor(device_a) == device_b - floor(device_b);
}

static band_change band_change_for(const decorations *d, const decor_paint *paint,
                                   const band_job *job)
{
    const band_mark *mark = &d->shown[job->band].mark;
    if (!mark->painted || mark->scale != paint->scale || !wm_rect_equals(&mark->rect, &job->rect)) {
        return BAND_CHANGED;
    }
    if ((shows_icon(paint->layout, &job->rect) || shows_icon(&d->painted_layout, &job->rect))
        && paint->icon != d->painted_icon) {
        return BAND_CHANGED;
    }
    if ((shows_title(paint->layout, &job->rect) || shows_title(&d->painted_layout, &job->rect))
        && title_changed(d, paint)) {
        return BAND_CHANGED;
    }
    if (!same_ops_in_band(&d->painted_layout, paint->layout, &job->rect)) {
        return BAND_CHANGED;
    }
    return decorations_same_alignment(mark->origin, paint->origin, paint->scale) ? BAND_SAME
                                                                                 : BAND_MOVED;
}

static bool shows_icon(const wm_decor_layout *layout, const wm_rect *band)
{
    return wm_rect_intersects(&layout->app_icon, band);
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

static void remember_icon(decorations *d, cairo_surface_t *icon)
{
    if (d->painted_icon == icon) {
        return;
    }
    if (d->painted_icon != NULL) {
        cairo_surface_destroy(d->painted_icon);
    }
    d->painted_icon = icon != NULL ? cairo_surface_reference(icon) : NULL;
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

static void repaint_band(decorations *d, const decor_paint *paint, const band_job *job)
{
    release_paint(&d->spare[job->band]);
    release_paint(&d->shown[job->band]);
    d->shown[job->band] = render_paint(d, paint, &job->rect, paint->origin);
    band_set_attach(&d->main, job->band, d->shown[job->band].buffer, &job->rect);
    attach_mirror(d, paint, job);
}

static void realign_band(decorations *d, const decor_paint *paint, const band_job *job)
{
    band_paint *shown = &d->shown[job->band];
    band_paint *spare = &d->spare[job->band];
    band_paint previous = *shown;
    if (spare_fits(spare, paint, job)) {
        *shown = *spare;
    } else {
        release_paint(spare);
        *shown = render_paint(d, paint, &job->rect, paint->origin);
    }
    *spare = previous;
    band_set_attach(&d->main, job->band, shown->buffer, &job->rect);
}

static void attach_mirror(decorations *d, const decor_paint *paint, const band_job *job)
{
    wm_point corner = {0, 0};
    if (d->mirror.tree == NULL) {
        return;
    }
    if (decorations_same_alignment(paint->origin, corner, paint->scale)) {
        band_set_attach(&d->mirror, job->band, d->shown[job->band].buffer, &job->rect);
        return;
    }
    band_paint mirror = render_paint(d, paint, &job->rect, corner);
    band_set_attach(&d->mirror, job->band, mirror.buffer, &job->rect);
    release_paint(&mirror);
}

static bool spare_fits(const band_paint *spare, const decor_paint *paint, const band_job *job)
{
    const band_mark *mark = &spare->mark;
    return spare->buffer != NULL && mark->scale == paint->scale
           && wm_rect_equals(&mark->rect, &job->rect)
           && decorations_same_alignment(mark->origin, paint->origin, paint->scale);
}

static band_paint render_paint(const decorations *d, const decor_paint *paint,
                               const wm_rect *rect, wm_point origin)
{
    cairo_surface_t *surface =
        wm_rect_is_empty(rect) ? NULL : render_band(d, paint, rect, origin);
    struct wlr_buffer *buffer = surface != NULL ? cairo_buffer_create(surface) : NULL;
    if (surface != NULL) {
        cairo_surface_destroy(surface);
    }
    return (band_paint){{buffer != NULL, *rect, paint->scale, origin}, buffer};
}

static void release_paint(band_paint *paint)
{
    if (paint->buffer != NULL) {
        wlr_buffer_drop(paint->buffer);
    }
    *paint = (band_paint){0};
}

static void release_paints(decorations *d)
{
    for (int i = 0; i < FRAME_BAND_COUNT; i++) {
        release_paint(&d->shown[i]);
        release_paint(&d->spare[i]);
    }
}

static bool band_set_create(band_set *set, struct wlr_scene_tree *parent)
{
    set->tree = wlr_scene_tree_create(parent);
    if (set->tree == NULL) {
        return false;
    }
    for (int i = 0; i < FRAME_BAND_COUNT; i++) {
        set->bands[i] = wlr_scene_buffer_create(set->tree, NULL);
    }
    wlr_scene_node_set_enabled(&set->tree->node, false);
    return true;
}

static void band_set_destroy(band_set *set)
{
    if (set->tree != NULL) {
        wlr_scene_node_destroy(&set->tree->node);
    }
    *set = (band_set){0};
}

static void band_set_show(band_set *set, bool shown)
{
    if (set->tree != NULL) {
        wlr_scene_node_set_enabled(&set->tree->node, shown);
    }
}

static void band_set_opacity(band_set *set, float opacity)
{
    for (int i = 0; i < FRAME_BAND_COUNT; i++) {
        if (set->bands[i] != NULL) {
            wlr_scene_buffer_set_opacity(set->bands[i], opacity);
        }
    }
}

static void band_set_attach(band_set *set, frame_band band, struct wlr_buffer *buffer,
                            const wm_rect *rect)
{
    struct wlr_scene_buffer *node = set->bands[band];
    if (node == NULL) {
        return;
    }
    wlr_scene_buffer_set_buffer(node, buffer);
    if (buffer != NULL) {
        wlr_scene_buffer_set_dest_size(node, rect->width, rect->height);
        wlr_scene_node_set_position(&node->node, rect->x, rect->y);
    }
    wlr_scene_node_set_enabled(&node->node, buffer != NULL);
}

static cairo_surface_t *render_band(const decorations *d, const decor_paint *paint,
                                    const wm_rect *rect, wm_point origin)
{
    frame_paint_request request = {
        .layout = paint->layout,
        .band = *rect,
        .scale = paint->scale,
        .title = paint->title,
        .icon = paint->icon,
        .origin = origin,
    };
    return d->painter->paint_frame(d->painter->self, &request);
}
