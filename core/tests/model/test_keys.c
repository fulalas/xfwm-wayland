#include "check.h"
#include "wm/keys.h"

static void test_plain_names_map_to_actions(void)
{
    CHECK_INT_EQ(WM_ACTION_CLOSE_WINDOW, wm_action_from_name("close_window_key"));
    CHECK_INT_EQ(WM_ACTION_CYCLE_WINDOWS, wm_action_from_name("cycle_windows_key"));
    CHECK_INT_EQ(WM_ACTION_CANCEL, wm_action_from_name("cancel_key"));
    CHECK_INT_EQ(WM_ACTION_TOGGLE_ABOVE, wm_action_from_name("above_key"));
    CHECK_INT_EQ(WM_ACTION_TOGGLE_FULLSCREEN, wm_action_from_name("fullscreen_key"));
    CHECK_INT_EQ(WM_ACTION_RESIZE, wm_action_from_name("resize_window_key"));
    CHECK_INT_EQ(WM_ACTION_MOVE, wm_action_from_name("move_window_key"));
    CHECK_INT_EQ(WM_ACTION_TILE_UP_RIGHT, wm_action_from_name("tile_up_right_key"));
}

static void test_monitor_and_workspace_moves_are_distinct(void)
{
    CHECK_INT_EQ(WM_ACTION_MOVE_TO_MONITOR_LEFT,
                 wm_action_from_name("move_window_to_monitor_left_key"));
    CHECK_INT_EQ(WM_ACTION_MOVE_LEFT_WORKSPACE,
                 wm_action_from_name("move_window_left_workspace_key"));
}

static void test_old_move_aliases_move_to_workspace(void)
{
    CHECK_INT_EQ(WM_ACTION_MOVE_LEFT_WORKSPACE, wm_action_from_name("move_window_left_key"));
    CHECK_INT_EQ(WM_ACTION_MOVE_RIGHT_WORKSPACE, wm_action_from_name("move_window_right_key"));
    CHECK_INT_EQ(WM_ACTION_MOVE_UP_WORKSPACE, wm_action_from_name("move_window_up_key"));
    CHECK_INT_EQ(WM_ACTION_MOVE_DOWN_WORKSPACE, wm_action_from_name("move_window_down_key"));
}

static void test_numbered_workspace_names(void)
{
    CHECK_INT_EQ(WM_ACTION_WORKSPACE_1, wm_action_from_name("workspace_1_key"));
    CHECK_INT_EQ(WM_ACTION_WORKSPACE_1 + 9, wm_action_from_name("workspace_10_key"));
    CHECK_INT_EQ(WM_ACTION_WORKSPACE_12, wm_action_from_name("workspace_12_key"));
}

static void test_numbered_move_to_workspace_names(void)
{
    CHECK_INT_EQ(WM_ACTION_MOVE_WORKSPACE_1, wm_action_from_name("move_window_workspace_1_key"));
    CHECK_INT_EQ(WM_ACTION_MOVE_WORKSPACE_1 + 2,
                 wm_action_from_name("move_window_workspace_3_key"));
    CHECK_INT_EQ(WM_ACTION_MOVE_WORKSPACE_12,
                 wm_action_from_name("move_window_workspace_12_key"));
}

static void test_out_of_range_workspace_numbers_are_unknown(void)
{
    CHECK_INT_EQ(WM_ACTION_NONE, wm_action_from_name("workspace_0_key"));
    CHECK_INT_EQ(WM_ACTION_NONE, wm_action_from_name("workspace_13_key"));
    CHECK_INT_EQ(WM_ACTION_NONE, wm_action_from_name("workspace_01_key"));
    CHECK_INT_EQ(WM_ACTION_NONE, wm_action_from_name("workspace_123_key"));
    CHECK_INT_EQ(WM_ACTION_NONE, wm_action_from_name("workspace__key"));
}

static void test_unknown_names_map_to_none(void)
{
    CHECK_INT_EQ(WM_ACTION_NONE, wm_action_from_name("xfce4-terminal"));
    CHECK_INT_EQ(WM_ACTION_NONE, wm_action_from_name(""));
    CHECK_INT_EQ(WM_ACTION_NONE, wm_action_from_name("close_window"));
    CHECK_INT_EQ(WM_ACTION_NONE, wm_action_from_name("workspace_1_keys"));
    CHECK_INT_EQ(WM_ACTION_NONE, wm_action_from_name(NULL));
}

static void test_global_keys_are_special_or_modified(void)
{
    wm_key volume = {0x1008ff13, 0};
    wm_key function = {0xffc9, 0};
    wm_key print = {0xff61, 0};
    wm_key copy = {0x63, WM_MOD_CTRL};
    CHECK(wm_key_is_global(&volume));
    CHECK(wm_key_is_global(&function));
    CHECK(wm_key_is_global(&print));
    CHECK(wm_key_is_global(&copy));
}

static void test_typing_keys_are_not_global(void)
{
    wm_key letter = {0x61, WM_MOD_SHIFT};
    wm_key enter = {0xff0d, 0};
    wm_key escape = {0xff1b, 0};
    wm_key shift = {0xffe1, 0};
    CHECK(!wm_key_is_global(&letter));
    CHECK(!wm_key_is_global(&enter));
    CHECK(!wm_key_is_global(&escape));
    CHECK(!wm_key_is_global(&shift));
}

static const check_case CASES[] = {
    {"plain_names_map_to_actions", test_plain_names_map_to_actions},
    {"monitor_and_workspace_moves_are_distinct",
     test_monitor_and_workspace_moves_are_distinct},
    {"old_move_aliases_move_to_workspace", test_old_move_aliases_move_to_workspace},
    {"numbered_workspace_names", test_numbered_workspace_names},
    {"numbered_move_to_workspace_names", test_numbered_move_to_workspace_names},
    {"out_of_range_workspace_numbers_are_unknown",
     test_out_of_range_workspace_numbers_are_unknown},
    {"unknown_names_map_to_none", test_unknown_names_map_to_none},
    {"global_keys_are_special_or_modified", test_global_keys_are_special_or_modified},
    {"typing_keys_are_not_global", test_typing_keys_are_not_global},
};

int main(void)
{
    return check_run(CASES, sizeof(CASES) / sizeof(CASES[0]));
}
