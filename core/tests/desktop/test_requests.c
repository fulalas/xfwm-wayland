#include "check.h"
#include "fake_ports.h"

static const wm_rect small = {0, 0, 400, 300};

static wm_desktop *start(fake_world *world)
{
    fake_world_init(world);
    wm_config config = fake_config();
    return fake_desktop_create(world, &config);
}

static void test_minimize_sends_no_configure(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    fake_map(desktop, 2, small);
    wm_window *window = fake_map(desktop, 1, small);
    int calls = world.configure_calls;
    wm_request_state(desktop, window, &(wm_state_change) {WM_STATE_MINIMIZED, 0});
    CHECK(wm_window_current_view(window).minimized);
    CHECK_INT_EQ(calls, world.configure_calls);
    wm_desktop_destroy(desktop);
}

static void test_refused_maximize_still_answers(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    wm_window_desc desc = fake_desc(1, small);
    desc.capabilities = WM_CAP_ALL & ~(uint32_t) WM_CAP_MAXIMIZE;
    wm_window *window = wm_window_map(desktop, &desc);
    int calls = world.configure_calls;
    wm_request_state(desktop, window, &(wm_state_change) {WM_STATE_MAXIMIZED, 0});
    CHECK(!wm_window_current_view(window).maximized);
    CHECK_INT_EQ(calls + 1, world.configure_calls);
    wm_desktop_destroy(desktop);
}

static void test_maximize_directions_are_absolute(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    wm_window *window = fake_map(desktop, 1, small);
    wm_request_state(desktop, window, &(wm_state_change) {WM_STATE_MAXIMIZED, 0});
    wm_state_change vertical = {WM_STATE_MAXIMIZED_VERT, WM_STATE_MAXIMIZED_HORZ};
    wm_request_state(desktop, window, &vertical);
    CHECK(!wm_window_current_view(window).maximized);
    CHECK_INT_EQ(WM_EDGE_TOP | WM_EDGE_BOTTOM, wm_window_current_view(window).tiled_edges);
    wm_desktop_destroy(desktop);
}

static void test_appearance_presents_each_window_once(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    fake_map(desktop, 1, small);
    fake_map(desktop, 2, small);
    int calls = world.window_changed_calls;
    wm_theme_metrics theme = fake_theme();
    wm_config config = fake_config();
    config.frame.show_app_icon = !config.frame.show_app_icon;
    wm_desktop_set_appearance(desktop, &theme, &config);
    CHECK_INT_EQ(calls + 2, world.window_changed_calls);
    wm_desktop_destroy(desktop);
}

static void test_workspace_request_moves_window(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    wm_window *window = fake_map(desktop, 1, small);
    wm_request_workspace(desktop, window, 2);
    CHECK_INT_EQ(2, wm_window_current_view(window).workspace);
    CHECK(!wm_window_current_view(window).visible);
    wm_desktop_destroy(desktop);
}

static void test_workspace_request_unsticks_window(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    wm_window *window = fake_map(desktop, 1, small);
    wm_request_state(desktop, window, &(wm_state_change) {WM_STATE_STICKY, 0});
    wm_request_workspace(desktop, window, 1);
    CHECK(!wm_window_current_view(window).sticky);
    CHECK_INT_EQ(1, wm_window_current_view(window).workspace);
    wm_desktop_destroy(desktop);
}

static void test_workspace_request_out_of_range_is_ignored(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    wm_window *window = fake_map(desktop, 1, small);
    wm_request_workspace(desktop, window, 99);
    CHECK_INT_EQ(0, wm_window_current_view(window).workspace);
    CHECK(wm_window_current_view(window).visible);
    wm_desktop_destroy(desktop);
}

int main(void)
{
    static const check_case cases[] = {
        {"minimize_sends_no_configure", test_minimize_sends_no_configure},
        {"refused_maximize_still_answers", test_refused_maximize_still_answers},
        {"maximize_directions_are_absolute", test_maximize_directions_are_absolute},
        {"appearance_presents_each_window_once", test_appearance_presents_each_window_once},
        {"workspace_request_moves_window", test_workspace_request_moves_window},
        {"workspace_request_unsticks_window", test_workspace_request_unsticks_window},
        {"workspace_request_out_of_range_is_ignored",
         test_workspace_request_out_of_range_is_ignored},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
