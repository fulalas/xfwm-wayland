#ifndef COMPOSITOR_DECORATIONS_H
#define COMPOSITOR_DECORATIONS_H

#include <cairo.h>

#include "compositor/painter.h"
#include "wm/decor.h"

struct wlr_scene_tree;

typedef struct decorations decorations;

typedef struct decor_paint {
    const wm_decor_layout *layout;
    const char *title;
    cairo_surface_t *icon;
    double scale;
} decor_paint;

decorations *decorations_create(struct wlr_scene_tree *parent, const compositor_painter *painter);
void decorations_destroy(decorations *decorations);
void decorations_paint(decorations *decorations, const decor_paint *paint);
void decorations_invalidate(decorations *decorations);
void decorations_hide(decorations *decorations);
void decorations_set_opacity(decorations *decorations, float opacity);
struct wlr_scene_tree *decorations_mirror(decorations *decorations, struct wlr_scene_tree *parent);
struct wlr_scene_tree *decorations_tree(const decorations *decorations);

#endif
