#ifndef COMPOSITOR_SCALED_SURFACE_H
#define COMPOSITOR_SCALED_SURFACE_H

#include "wm/geometry.h"

#include <stdbool.h>

struct wlr_scene_buffer;
struct wlr_scene_tree;
struct wlr_surface;

typedef struct scaled_surfaces scaled_surfaces;

typedef struct scaled_surface_sizer {
    wm_size (*size)(void *data, struct wlr_surface *surface, wm_size buffer);
    void *data;
} scaled_surface_sizer;

scaled_surfaces *scaled_surfaces_create(scaled_surface_sizer sizer);
void scaled_surfaces_destroy(scaled_surfaces *set);
struct wlr_scene_tree *scaled_surfaces_add(scaled_surfaces *set, struct wlr_scene_tree *parent,
                                           struct wlr_surface *surface);
void scaled_surfaces_set_scale(scaled_surfaces *set, double scale);
void scaled_surfaces_refit(struct wlr_scene_tree *tree);
bool scaled_surfaces_to_local(struct wlr_scene_buffer *buffer, double *sx, double *sy);

#endif
