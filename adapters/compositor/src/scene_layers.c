#include <stdlib.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/util/log.h>

#include "scene_layers.h"

struct scene_layers {
    struct wlr_scene_tree *trees[SCENE_LAYER_COUNT];
};

scene_layers *scene_layers_create(struct wlr_scene *scene)
{
    scene_layers *layers = calloc(1, sizeof(*layers));
    if (layers == NULL) {
        return NULL;
    }
    for (int i = 0; i < SCENE_LAYER_COUNT; i++) {
        layers->trees[i] = wlr_scene_tree_create(&scene->tree);
        if (layers->trees[i] == NULL) {
            wlr_log(WLR_ERROR, "cannot create scene layer %d", i);
            free(layers);
            return NULL;
        }
    }
    return layers;
}

void scene_layers_destroy(scene_layers *layers)
{
    free(layers);
}

struct wlr_scene_tree *scene_layers_tree(const scene_layers *layers, scene_layer layer)
{
    return layers->trees[layer];
}
