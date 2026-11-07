#include "layer_mapping.h"
#include "wlr-layer-shell-unstable-v1-protocol.h"

scene_layer layer_mapping_from_wm(wm_layer layer)
{
    switch (layer) {
    case WM_LAYER_DESKTOP:
        return SCENE_LAYER_DESKTOP;
    case WM_LAYER_BELOW:
        return SCENE_LAYER_BELOW;
    case WM_LAYER_ABOVE:
        return SCENE_LAYER_ABOVE;
    case WM_LAYER_DOCK:
        return SCENE_LAYER_DOCK;
    case WM_LAYER_FULLSCREEN:
        return SCENE_LAYER_FULLSCREEN;
    case WM_LAYER_NORMAL:
    case WM_LAYER_COUNT:
        break;
    }
    return SCENE_LAYER_NORMAL;
}

scene_layer layer_mapping_from_shell(uint32_t shell_layer)
{
    switch (shell_layer) {
    case ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND:
        return SCENE_LAYER_BACKGROUND;
    case ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM:
        return SCENE_LAYER_BOTTOM;
    case ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY:
        return SCENE_LAYER_OVERLAY;
    default:
        break;
    }
    return SCENE_LAYER_TOP;
}
