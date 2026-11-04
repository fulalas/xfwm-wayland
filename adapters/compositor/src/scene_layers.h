#ifndef COMPOSITOR_SCENE_LAYERS_H
#define COMPOSITOR_SCENE_LAYERS_H

#include "layer_mapping.h"

struct wlr_scene;
struct wlr_scene_tree;

typedef struct scene_layers scene_layers;

scene_layers *scene_layers_create(struct wlr_scene *scene);
void scene_layers_destroy(scene_layers *layers);
struct wlr_scene_tree *scene_layers_tree(const scene_layers *layers, scene_layer layer);

#endif
