#include "check.h"
#include "fake_input.h"

static void test_saved_workspace_is_used(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window_desc desc = fake_desc_at(1, (wm_rect) {100, 100, 400, 300});
    desc.has_workspace = true;
    desc.workspace = 2;
    wm_window *window = wm_window_map(desktop, &desc);
    wm_window_view view = wm_window_current_view(window);
    CHECK_INT_EQ(2, view.workspace);
    CHECK(!view.activated);
    CHECK_INT_EQ(0, world.workspaces.current);
    wm_desktop_destroy(desktop);
}

static void test_unknown_saved_workspace_falls_back_to_current(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_desktop_switch_workspace(desktop, 1);
    wm_window_desc desc = fake_desc_at(1, (wm_rect) {100, 100, 400, 300});
    desc.has_workspace = true;
    desc.workspace = 9;
    wm_window *window = wm_window_map(desktop, &desc);
    CHECK_INT_EQ(1, wm_window_current_view(window).workspace);
    wm_desktop_destroy(desktop);
}

static void test_transient_ignores_saved_workspace(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *parent = fake_map(desktop, 1, (wm_rect) {0, 0, 400, 300});
    wm_window_desc desc = fake_dialog_desc(2, parent);
    desc.has_workspace = true;
    desc.workspace = 3;
    wm_window *dialog = wm_window_map(desktop, &desc);
    CHECK_INT_EQ(0, wm_window_current_view(dialog).workspace);
    wm_desktop_destroy(desktop);
}

static void test_saved_geometry_is_kept(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map_at(desktop, 1, (wm_rect) {120, 140, 400, 300});
    CHECK_RECT_EQ(((wm_rect) {120, 140, 400, 300}), fake_client_of(window));
    CHECK_RECT_EQ(((wm_rect) {120, 140, 400, 300}), wm_window_current_view(window).restore_client);
    wm_desktop_destroy(desktop);
}

static void test_saved_maximized_window_restores_to_saved_geometry(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window_desc desc = fake_desc_at(1, (wm_rect) {120, 140, 400, 300});
    desc.states = WM_STATE_MAXIMIZED;
    wm_window *window = wm_window_map(desktop, &desc);
    wm_window_view view = wm_window_current_view(window);
    CHECK(view.maximized);
    CHECK_RECT_EQ(((wm_rect) {120, 140, 400, 300}), view.restore_client);
    wm_state_change change = {.unset = WM_STATE_MAXIMIZED};
    wm_request_state(desktop, window, &change);
    CHECK_RECT_EQ(((wm_rect) {120, 140, 400, 300}), fake_client_of(window));
    wm_desktop_destroy(desktop);
}

static void test_fullscreen_window_reports_geometry_before_fullscreen(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map_at(desktop, 1, (wm_rect) {120, 140, 400, 300});
    wm_state_change change = {.set = WM_STATE_FULLSCREEN};
    wm_request_state(desktop, window, &change);
    wm_window_view view = wm_window_current_view(window);
    CHECK(view.fullscreen);
    CHECK_RECT_EQ(((wm_rect) {120, 140, 400, 300}), view.restore_client);
    wm_desktop_destroy(desktop);
}

static const check_case CASES[] = {
    {"saved_workspace_is_used", test_saved_workspace_is_used},
    {"unknown_saved_workspace_falls_back_to_current",
     test_unknown_saved_workspace_falls_back_to_current},
    {"transient_ignores_saved_workspace", test_transient_ignores_saved_workspace},
    {"saved_geometry_is_kept", test_saved_geometry_is_kept},
    {"saved_maximized_window_restores_to_saved_geometry",
     test_saved_maximized_window_restores_to_saved_geometry},
    {"fullscreen_window_reports_geometry_before_fullscreen",
     test_fullscreen_window_reports_geometry_before_fullscreen},
};

int main(void)
{
    return check_run(CASES, sizeof(CASES) / sizeof(CASES[0]));
}
