#include "check.h"
#include "layer_mapping.h"
#include "wlr-layer-shell-unstable-v1-protocol.h"

static void test_core_layers_keep_their_order(void)
{
    CHECK(layer_mapping_from_wm(WM_LAYER_DESKTOP) < layer_mapping_from_wm(WM_LAYER_BELOW));
    CHECK(layer_mapping_from_wm(WM_LAYER_BELOW) < layer_mapping_from_wm(WM_LAYER_NORMAL));
    CHECK(layer_mapping_from_wm(WM_LAYER_NORMAL) < layer_mapping_from_wm(WM_LAYER_ABOVE));
    CHECK(layer_mapping_from_wm(WM_LAYER_ABOVE) < layer_mapping_from_wm(WM_LAYER_DOCK));
    CHECK(layer_mapping_from_wm(WM_LAYER_DOCK) < layer_mapping_from_wm(WM_LAYER_FULLSCREEN));
}

static void test_shell_layers_surround_windows(void)
{
    CHECK(layer_mapping_from_shell(ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND) <
          layer_mapping_from_wm(WM_LAYER_DESKTOP));
    CHECK(layer_mapping_from_shell(ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM) <
          layer_mapping_from_wm(WM_LAYER_DESKTOP));
    CHECK(layer_mapping_from_shell(ZWLR_LAYER_SHELL_V1_LAYER_TOP) >
          layer_mapping_from_wm(WM_LAYER_DOCK));
    CHECK(layer_mapping_from_shell(ZWLR_LAYER_SHELL_V1_LAYER_TOP) <
          layer_mapping_from_wm(WM_LAYER_FULLSCREEN));
}

static void test_overlay_is_above_fullscreen_and_unmanaged(void)
{
    scene_layer overlay = layer_mapping_from_shell(ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY);
    CHECK(overlay > layer_mapping_from_wm(WM_LAYER_FULLSCREEN));
    CHECK(overlay > SCENE_LAYER_UNMANAGED);
    CHECK(overlay < SCENE_LAYER_OSD);
}

static void test_lock_is_the_top_layer(void)
{
    CHECK_INT_EQ(SCENE_LAYER_COUNT - 1, SCENE_LAYER_LOCK);
    CHECK(SCENE_LAYER_DRAG < SCENE_LAYER_LOCK);
}

int main(void)
{
    const check_case cases[] = {
        {"core_layers_keep_their_order", test_core_layers_keep_their_order},
        {"shell_layers_surround_windows", test_shell_layers_surround_windows},
        {"overlay_is_above_fullscreen_and_unmanaged",
         test_overlay_is_above_fullscreen_and_unmanaged},
        {"lock_is_the_top_layer", test_lock_is_the_top_layer},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
