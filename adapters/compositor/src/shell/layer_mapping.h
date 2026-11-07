#ifndef COMPOSITOR_LAYER_MAPPING_H
#define COMPOSITOR_LAYER_MAPPING_H

#include <stdbool.h>
#include <stdint.h>

#include "wm/views.h"

typedef enum scene_layer {
    SCENE_LAYER_BACKGROUND = 0,
    SCENE_LAYER_BOTTOM,
    SCENE_LAYER_DESKTOP,
    SCENE_LAYER_BELOW,
    SCENE_LAYER_NORMAL,
    SCENE_LAYER_ABOVE,
    SCENE_LAYER_DOCK,
    SCENE_LAYER_TOP,
    SCENE_LAYER_FULLSCREEN,
    SCENE_LAYER_UNMANAGED,
    SCENE_LAYER_OVERLAY,
    SCENE_LAYER_POPUPS,
    SCENE_LAYER_OSD,
    SCENE_LAYER_DRAG,
    SCENE_LAYER_LOCK,
    SCENE_LAYER_COUNT
} scene_layer;

scene_layer layer_mapping_from_wm(wm_layer layer);
scene_layer layer_mapping_from_shell(uint32_t shell_layer);
bool layer_mapping_is_desktop(scene_layer layer, const char *name_space);

#endif
