#include <string.h>

#include "check.h"
#include "fake_input.h"

static const wm_rect small = {0, 0, 400, 300};

static void test_switch_restores_workspace_focus(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *first = fake_map(desktop, 1, small);
    wm_desktop_switch_workspace(desktop, 1);
    CHECK(!wm_window_current_view(first).visible);
    wm_window *second = fake_map(desktop, 2, small);
    wm_desktop_switch_workspace(desktop, 0);
    CHECK(wm_window_current_view(first).visible);
    CHECK(!wm_window_current_view(second).visible);
    CHECK(world.focused == fake_handle(1));
    wm_desktop_destroy(desktop);
}

static void test_move_window_to_workspace_key(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    fake_map(desktop, 1, small);
    wm_window *second = fake_map(desktop, 2, small);
    fake_key(desktop, FAKE_KEY_2, WM_MOD_ALT | WM_MOD_CTRL);
    CHECK_INT_EQ(1, wm_window_current_view(second).workspace);
    CHECK(!wm_window_current_view(second).visible);
    CHECK(world.focused == fake_handle(1));
    wm_desktop_destroy(desktop);
}

static void test_left_workspace_wraps_layout(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    fake_key(desktop, WM_KEYSYM_LEFT, WM_MOD_CTRL | WM_MOD_ALT);
    CHECK_INT_EQ(3, world.workspaces.current);
    fake_key(desktop, WM_KEYSYM_RIGHT, WM_MOD_CTRL | WM_MOD_ALT);
    CHECK_INT_EQ(0, world.workspaces.current);
    wm_desktop_destroy(desktop);
}

static void test_left_workspace_stops_without_wrap(void)
{
    fake_world world;
    wm_config config = fake_config();
    config.workspaces.wrap_layout = false;
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    fake_key(desktop, WM_KEYSYM_LEFT, WM_MOD_CTRL | WM_MOD_ALT);
    CHECK_INT_EQ(0, world.workspaces.current);
    wm_desktop_destroy(desktop);
}

static void test_toggle_returns_to_previous(void)
{
    fake_world world;
    wm_config config = fake_config();
    config.workspaces.toggle = true;
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    fake_key(desktop, FAKE_KEY_2, WM_MOD_CTRL);
    CHECK_INT_EQ(1, world.workspaces.current);
    fake_key(desktop, FAKE_KEY_2, WM_MOD_CTRL);
    CHECK_INT_EQ(0, world.workspaces.current);
    wm_desktop_destroy(desktop);
}

static void test_switch_out_of_range_without_wrap_cycle(void)
{
    fake_world world;
    wm_config config = fake_config();
    config.workspaces.wrap_cycle = false;
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_desktop_switch_workspace(desktop, 7);
    CHECK_INT_EQ(0, world.workspaces.current);
    wm_desktop_destroy(desktop);
}

static void test_add_and_delete_active_workspace(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    fake_key(desktop, FAKE_KEY_A, WM_MOD_SUPER);
    CHECK_INT_EQ(5, world.workspaces.count);
    CHECK_INT_EQ(5, world.stored_workspace_count);
    wm_desktop_switch_workspace(desktop, 2);
    wm_window *window = fake_map(desktop, 1, small);
    wm_desktop_switch_workspace(desktop, 1);
    fake_key(desktop, FAKE_KEY_X, WM_MOD_SUPER);
    CHECK_INT_EQ(4, world.workspaces.count);
    CHECK_INT_EQ(1, wm_window_current_view(window).workspace);
    CHECK(wm_window_current_view(window).visible);
    wm_desktop_destroy(desktop);
}

static void test_workspace_names(void)
{
    fake_world world;
    wm_config config = fake_config();
    strcpy(config.workspaces.names[1], "Mail");
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    CHECK(world.workspaces.names[0] == NULL);
    CHECK(strcmp(world.workspaces.names[1], "Mail") == 0);
    wm_desktop_destroy(desktop);
}

static void test_fewer_workspaces_from_config_moves_windows(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_desktop_switch_workspace(desktop, 3);
    wm_window *window = fake_map(desktop, 1, small);
    config.workspaces.count = 2;
    wm_theme_metrics theme = fake_theme();
    wm_desktop_set_appearance(desktop, &theme, &config);
    CHECK_INT_EQ(2, world.workspaces.count);
    CHECK_INT_EQ(1, world.workspaces.current);
    CHECK_INT_EQ(1, wm_window_current_view(window).workspace);
    CHECK_INT_EQ(0, world.store_calls);
    wm_desktop_destroy(desktop);
}

static void test_show_desktop_toggle(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    fake_key(desktop, FAKE_KEY_D, WM_MOD_CTRL | WM_MOD_ALT);
    CHECK(!wm_window_current_view(window).visible);
    CHECK(world.focused == NULL);
    fake_key(desktop, FAKE_KEY_D, WM_MOD_CTRL | WM_MOD_ALT);
    CHECK(wm_window_current_view(window).visible);
    CHECK(world.focused == fake_handle(1));
    wm_desktop_destroy(desktop);
}

static void test_scroll_on_desktop_switches_workspace(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    fake_click where = {WM_TARGET_DESKTOP, NULL, {50, 50}, 0, 0};
    fake_scroll(&world, desktop, &where, 1);
    CHECK_INT_EQ(1, world.workspaces.current);
    fake_scroll(&world, desktop, &where, 1);
    CHECK_INT_EQ(1, world.workspaces.current);
    fake_advance(&world, desktop, 30);
    fake_scroll(&world, desktop, &where, -1);
    CHECK_INT_EQ(0, world.workspaces.current);
    wm_desktop_destroy(desktop);
}

int main(void)
{
    static const check_case cases[] = {
        {"switch_restores_workspace_focus", test_switch_restores_workspace_focus},
        {"move_window_to_workspace_key", test_move_window_to_workspace_key},
        {"left_workspace_wraps_layout", test_left_workspace_wraps_layout},
        {"left_workspace_stops_without_wrap", test_left_workspace_stops_without_wrap},
        {"toggle_returns_to_previous", test_toggle_returns_to_previous},
        {"switch_out_of_range_without_wrap_cycle", test_switch_out_of_range_without_wrap_cycle},
        {"add_and_delete_active_workspace", test_add_and_delete_active_workspace},
        {"workspace_names", test_workspace_names},
        {"fewer_workspaces_from_config_moves_windows",
         test_fewer_workspaces_from_config_moves_windows},
        {"show_desktop_toggle", test_show_desktop_toggle},
        {"scroll_on_desktop_switches_workspace", test_scroll_on_desktop_switches_workspace},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
