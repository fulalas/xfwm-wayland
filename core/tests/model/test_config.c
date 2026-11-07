#include <string.h>

#include "check.h"
#include "wm/config.h"
#include "wm/keys.h"

static void test_frame_defaults_match_xfwm4(void)
{
    wm_frame_config frame = wm_config_defaults().frame;
    CHECK(strcmp(frame.button_layout, "O|SHMC") == 0);
    CHECK_INT_EQ(0, frame.button_offset);
    CHECK_INT_EQ(0, frame.button_spacing);
    CHECK_INT_EQ(0, frame.border_top);
    CHECK(frame.borderless_maximize);
    CHECK(!frame.titleless_maximize);
    CHECK(!frame.show_app_icon);
}

static void test_title_defaults_are_centered_full_width(void)
{
    wm_title_config title = wm_config_defaults().frame.title;
    CHECK_INT_EQ(WM_ALIGN_CENTER, title.alignment);
    CHECK(title.full_width);
    CHECK_INT_EQ(WM_TITLE_SHADOW_NONE, title.shadow_active);
    CHECK_INT_EQ(WM_TITLE_SHADOW_NONE, title.shadow_inactive);
}

static void test_focus_defaults_are_click_to_focus(void)
{
    wm_focus_config focus = wm_config_defaults().focus;
    CHECK(focus.click_to_focus);
    CHECK_INT_EQ(250, focus.focus_delay_ms);
    CHECK_INT_EQ(250, focus.raise_delay_ms);
    CHECK(!focus.raise_on_focus);
    CHECK(focus.raise_on_click);
    CHECK_INT_EQ(WM_ACTIVATE_BRING, focus.activate_action);
}

static void test_easy_click_defaults_to_alt(void)
{
    wm_mouse_config mouse = wm_config_defaults().mouse;
    CHECK_INT_EQ(WM_MOD_ALT, mouse.easy_click_modifiers);
    CHECK_INT_EQ(WM_DOUBLE_CLICK_MAXIMIZE, mouse.double_click_action);
    CHECK_INT_EQ(250, mouse.double_click_time_ms);
    CHECK_INT_EQ(5, mouse.double_click_distance);
}

static void test_move_defaults_snap_to_border(void)
{
    wm_move_config move = wm_config_defaults().move;
    CHECK(move.snap_to_border);
    CHECK(!move.snap_to_windows);
    CHECK_INT_EQ(10, move.snap_width);
    CHECK(move.tile_on_move);
    CHECK(move.wrap_windows);
    CHECK_INT_EQ(10, move.wrap_resistance);
}

static void test_placement_defaults_to_center_without_margins(void)
{
    wm_placement_config placement = wm_config_defaults().placement;
    CHECK_INT_EQ(WM_PLACE_CENTER, placement.mode);
    CHECK_INT_EQ(20, placement.ratio_percent);
    CHECK_INT_EQ(0, placement.margins.top + placement.margins.bottom + placement.margins.left
                        + placement.margins.right);
}

static void test_cycle_defaults_show_preview_grid(void)
{
    wm_cycle_config cycle = wm_config_defaults().cycle;
    CHECK(cycle.preview);
    CHECK(cycle.draw_frame);
    CHECK(cycle.hidden);
    CHECK(cycle.minimum);
    CHECK(!cycle.minimized);
    CHECK_INT_EQ(WM_TABWIN_GRID, cycle.tabwin_mode);
}

static void test_four_unnamed_workspaces(void)
{
    wm_workspace_config workspaces = wm_config_defaults().workspaces;
    CHECK_INT_EQ(4, workspaces.count);
    CHECK(workspaces.wrap_cycle);
    for (int i = 0; i < WM_WORKSPACE_MAX; i++) {
        CHECK(workspaces.names[i][0] == '\0');
    }
}

static void test_compositing_defaults_enabled_with_gl(void)
{
    wm_compositing_config compositing = wm_config_defaults().compositing;
    CHECK(compositing.enabled);
    CHECK(compositing.use_gl);
    CHECK_INT_EQ(WM_VBLANK_AUTO, compositing.vblank_mode);
    CHECK_INT_EQ(50, compositing.shadow_opacity_percent);
    CHECK_INT_EQ(100, compositing.frame_opacity_percent);
    CHECK(compositing.show_frame_shadow);
    CHECK(!compositing.show_popup_shadow);
}

static void test_shadow_deltas_default_to_three_pixels_up(void)
{
    wm_compositing_config compositing = wm_config_defaults().compositing;
    CHECK_INT_EQ(0, compositing.shadow_delta_x);
    CHECK_INT_EQ(-3, compositing.shadow_delta_y);
    CHECK_INT_EQ(0, compositing.shadow_delta_width);
    CHECK_INT_EQ(0, compositing.shadow_delta_height);
}

static const check_case CASES[] = {
    {"frame_defaults_match_xfwm4", test_frame_defaults_match_xfwm4},
    {"title_defaults_are_centered_full_width", test_title_defaults_are_centered_full_width},
    {"focus_defaults_are_click_to_focus", test_focus_defaults_are_click_to_focus},
    {"easy_click_defaults_to_alt", test_easy_click_defaults_to_alt},
    {"move_defaults_snap_to_border", test_move_defaults_snap_to_border},
    {"placement_defaults_to_center_without_margins",
     test_placement_defaults_to_center_without_margins},
    {"cycle_defaults_show_preview_grid", test_cycle_defaults_show_preview_grid},
    {"four_unnamed_workspaces", test_four_unnamed_workspaces},
    {"compositing_defaults_enabled_with_gl", test_compositing_defaults_enabled_with_gl},
    {"shadow_deltas_default_to_three_pixels_up", test_shadow_deltas_default_to_three_pixels_up},
};

int main(void)
{
    return check_run(CASES, sizeof(CASES) / sizeof(CASES[0]));
}
