#ifndef COMPOSITOR_SHADOW_H
#define COMPOSITOR_SHADOW_H

#include "shadow_cache.h"
#include "wm/geometry.h"

struct wlr_scene_tree;

typedef struct shadow shadow;

shadow *shadow_create(struct wlr_scene_tree *parent);
void shadow_destroy(shadow *shadow);
void shadow_release(shadow *shadow);
void shadow_show(shadow *shadow, const shadow_image *image, const wm_rect *rect);
void shadow_show_around(shadow *shadow, compositor *server, const wm_rect *frame);
void shadow_hide(shadow *shadow);
void shadow_set_opacity(shadow *shadow, float opacity);

#endif
