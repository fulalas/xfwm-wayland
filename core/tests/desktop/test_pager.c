#include "check.h"
#include "fake_input.h"

static const wm_rect small = {100, 100, 400, 300};
static const wm_rect second_screen = {FAKE_SCREEN_WIDTH, 0, FAKE_SCREEN_WIDTH, FAKE_SCREEN_HEIGHT};

static wm_desktop *start(fake_world *world)
{
    wm_config config = fake_config();
    return fake_desktop_start(world, &config);
}

static wm_desktop *start_dual(fake_world *world)
{
    fake_world_init(world);
    fake_world_add_output(world, second_screen, second_screen);
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_create(world, &config);
    fake_set_keymap(desktop);
    return desktop;
}

static void minimize(wm_desktop *desktop, wm_window *window)
{
    wm_request_state(desktop, window, &(wm_state_change) {WM_STATE_MINIMIZED, 0});
}

static void test_workspace_id_follows_workspace(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    wm_window *window = fake_map(desktop, 1, small);
    wm_request_workspace(desktop, window, 2);
    unsigned id = world.workspaces.ids[2];
    wm_desktop_remove_workspace(desktop, 0);
    CHECK_INT_EQ(1, wm_window_current_view(window).workspace);
    CHECK_INT_EQ(id, wm_window_current_view(window).workspace_id);
    wm_desktop_destroy(desktop);
}

static void test_send_to_monitor_moves_window(void)
{
    fake_world world;
    wm_desktop *desktop = start_dual(&world);
    wm_window *window = fake_map(desktop, 1, small);
    wm_request_send_to_monitor(desktop, window, 1);
    wm_rect client = wm_window_current_view(window).client;
    CHECK(client.x >= FAKE_SCREEN_WIDTH);
    CHECK_INT_EQ(small.width, client.width);
    wm_desktop_destroy(desktop);
}

static void test_send_to_current_monitor_keeps_window(void)
{
    fake_world world;
    wm_desktop *desktop = start_dual(&world);
    wm_window *window = fake_map(desktop, 1, small);
    wm_rect before = wm_window_current_view(window).client;
    wm_request_send_to_monitor(desktop, window, 0);
    wm_request_send_to_monitor(desktop, window, 5);
    CHECK_RECT_EQ(before, wm_window_current_view(window).client);
    wm_desktop_destroy(desktop);
}

static void test_keyboard_move_starts_at_window_center(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    wm_window *window = fake_map(desktop, 1, small);
    int warps = world.warp_calls;
    wm_request_keyboard_move(desktop, window);
    CHECK(wm_window_current_view(window).moving);
    CHECK_INT_EQ(warps + 1, world.warp_calls);
    wm_desktop_destroy(desktop);
}

static void test_keyboard_move_ignores_hidden_window(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    wm_window *window = fake_map(desktop, 1, small);
    minimize(desktop, window);
    wm_request_keyboard_move(desktop, window);
    CHECK(!wm_window_current_view(window).moving);
    wm_desktop_destroy(desktop);
}

static void test_keyboard_resize_starts_resize(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    wm_window *window = fake_map(desktop, 1, small);
    wm_request_keyboard_resize(desktop, window);
    CHECK(wm_window_current_view(window).resizing);
    wm_desktop_destroy(desktop);
}

static void test_highlight_outlines_frame(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    wm_window *window = fake_map(desktop, 1, small);
    wm_request_highlight(desktop, window);
    CHECK(world.outline_visible);
    CHECK_RECT_EQ(wm_window_current_view(window).frame, world.outline);
    wm_desktop_destroy(desktop);
}

static void test_unhighlight_hides_outline(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    wm_window *window = fake_map(desktop, 1, small);
    wm_request_highlight(desktop, window);
    wm_request_unhighlight(desktop);
    CHECK(!world.outline_visible);
    wm_desktop_destroy(desktop);
}

static void test_highlight_moves_to_new_window(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    wm_window *first = fake_map(desktop, 1, small);
    wm_window *second = fake_map(desktop, 2, (wm_rect) {700, 400, 300, 200});
    wm_request_highlight(desktop, first);
    wm_request_highlight(desktop, second);
    CHECK_RECT_EQ(wm_window_current_view(second).frame, world.outline);
    wm_desktop_destroy(desktop);
}

static void test_highlight_follows_window(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    wm_window *window = fake_map(desktop, 1, small);
    wm_request_highlight(desktop, window);
    wm_request_configure(desktop, window, &(wm_rect) {300, 250, 400, 300});
    CHECK_RECT_EQ(wm_window_current_view(window).frame, world.outline);
    wm_desktop_destroy(desktop);
}

static void test_highlight_hidden_while_minimized(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    wm_window *window = fake_map(desktop, 1, small);
    wm_request_highlight(desktop, window);
    minimize(desktop, window);
    CHECK(!world.outline_visible);
    wm_desktop_destroy(desktop);
}

static void test_highlight_ends_when_window_unmaps(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    fake_map(desktop, 2, small);
    wm_window *window = fake_map(desktop, 1, small);
    wm_request_highlight(desktop, window);
    wm_window_unmap(desktop, window);
    CHECK(!world.outline_visible);
    wm_desktop_destroy(desktop);
}

static const check_case CASES[] = {
    {"workspace_id_follows_workspace", test_workspace_id_follows_workspace},
    {"send_to_monitor_moves_window", test_send_to_monitor_moves_window},
    {"send_to_current_monitor_keeps_window", test_send_to_current_monitor_keeps_window},
    {"keyboard_move_starts_at_window_center", test_keyboard_move_starts_at_window_center},
    {"keyboard_move_ignores_hidden_window", test_keyboard_move_ignores_hidden_window},
    {"keyboard_resize_starts_resize", test_keyboard_resize_starts_resize},
    {"highlight_outlines_frame", test_highlight_outlines_frame},
    {"unhighlight_hides_outline", test_unhighlight_hides_outline},
    {"highlight_moves_to_new_window", test_highlight_moves_to_new_window},
    {"highlight_follows_window", test_highlight_follows_window},
    {"highlight_hidden_while_minimized", test_highlight_hidden_while_minimized},
    {"highlight_ends_when_window_unmaps", test_highlight_ends_when_window_unmaps},
};

int main(void)
{
    return check_run(CASES, sizeof(CASES) / sizeof(CASES[0]));
}
