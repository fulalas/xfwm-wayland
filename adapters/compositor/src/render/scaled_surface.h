#ifndef COMPOSITOR_SCALED_SURFACE_H
#define COMPOSITOR_SCALED_SURFACE_H

struct wlr_scene_tree;
struct wlr_surface;

typedef struct scaled_surfaces scaled_surfaces;

scaled_surfaces *scaled_surfaces_create(void);
void scaled_surfaces_destroy(scaled_surfaces *set);
struct wlr_scene_tree *scaled_surfaces_add(scaled_surfaces *set, struct wlr_scene_tree *parent,
                                           struct wlr_surface *surface);
void scaled_surfaces_set_scale(scaled_surfaces *set, double scale);

#endif
