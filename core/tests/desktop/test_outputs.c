#include "check.h"
#include "fake_input.h"

static const wm_rect second = {1920, 0, 1280, 1024};
static const wm_rect on_second = {2200, 200, 400, 300};

static wm_desktop *start(fake_world *world)
{
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(world, &config);
    fake_world_add_output(world, second, second);
    wm_desktop_outputs_changed(desktop);
    return desktop;
}

static void set_outputs(fake_world *world, wm_desktop *desktop, size_t count)
{
    world->output_count = count;
    wm_desktop_outputs_changed(desktop);
}

static void test_windows_stay_while_no_output(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    set_outputs(&world, desktop, 1);
    wm_window *window = fake_map_at(desktop, 1, (wm_rect) {600, 400, 400, 300});
    wm_window *maximized = fake_map_at(desktop, 2, (wm_rect) {100, 100, 400, 300});
    wm_request_state(desktop, maximized, &(wm_state_change) {WM_STATE_MAXIMIZED, 0});
    wm_rect where = fake_client_of(window);
    wm_rect full = fake_client_of(maximized);
    set_outputs(&world, desktop, 0);
    CHECK_RECT_EQ(where, fake_client_of(window));
    CHECK_RECT_EQ(full, fake_client_of(maximized));
    set_outputs(&world, desktop, 1);
    CHECK_RECT_EQ(where, fake_client_of(window));
    CHECK_RECT_EQ(full, fake_client_of(maximized));
    wm_desktop_destroy(desktop);
}

static void test_window_returns_to_readded_output(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    wm_window *window = fake_map_at(desktop, 1, on_second);
    set_outputs(&world, desktop, 1);
    CHECK(fake_client_of(window).x < FAKE_SCREEN_WIDTH);
    set_outputs(&world, desktop, 2);
    CHECK_RECT_EQ(on_second, fake_client_of(window));
    wm_desktop_destroy(desktop);
}

static void test_client_resize_keeps_return(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    wm_window *window = fake_map_at(desktop, 1, on_second);
    set_outputs(&world, desktop, 1);
    wm_rect resized = fake_client_of(window);
    resized.width = 500;
    wm_request_configure(desktop, window, &resized);
    set_outputs(&world, desktop, 2);
    CHECK_RECT_EQ(((wm_rect) {2200, 200, 500, 300}), fake_client_of(window));
    wm_desktop_destroy(desktop);
}

static void test_moved_window_stays_after_output_returns(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    wm_window *window = fake_map_at(desktop, 1, on_second);
    set_outputs(&world, desktop, 1);
    wm_request_keyboard_move(desktop, window);
    fake_key(desktop, WM_KEYSYM_DOWN, 0);
    fake_key(desktop, WM_KEYSYM_RETURN, 0);
    wm_rect moved = fake_client_of(window);
    set_outputs(&world, desktop, 2);
    CHECK_RECT_EQ(moved, fake_client_of(window));
    wm_desktop_destroy(desktop);
}

static void test_maximized_window_returns_with_saved_geometry(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    wm_window *window = fake_map_at(desktop, 1, on_second);
    wm_request_state(desktop, window, &(wm_state_change) {WM_STATE_MAXIMIZED, 0});
    wm_rect maximized = fake_client_of(window);
    set_outputs(&world, desktop, 1);
    CHECK(wm_window_current_view(window).restore_client.x < FAKE_SCREEN_WIDTH);
    set_outputs(&world, desktop, 2);
    CHECK_RECT_EQ(maximized, fake_client_of(window));
    wm_request_state(desktop, window, &(wm_state_change) {.unset = WM_STATE_MAXIMIZED});
    CHECK_RECT_EQ(on_second, fake_client_of(window));
    wm_desktop_destroy(desktop);
}

static void test_output_back_at_other_size_drops_return(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    wm_window *window = fake_map_at(desktop, 1, on_second);
    set_outputs(&world, desktop, 1);
    wm_rect evacuated = fake_client_of(window);
    world.outputs[1].full = (wm_rect) {1920, 0, 2560, 1440};
    world.outputs[1].usable = world.outputs[1].full;
    set_outputs(&world, desktop, 2);
    CHECK_RECT_EQ(evacuated, fake_client_of(window));
    set_outputs(&world, desktop, 1);
    world.outputs[1].full = second;
    world.outputs[1].usable = second;
    set_outputs(&world, desktop, 2);
    CHECK_RECT_EQ(evacuated, fake_client_of(window));
    wm_desktop_destroy(desktop);
}

static void test_maximize_while_evacuated_stays(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    wm_window *window = fake_map_at(desktop, 1, on_second);
    set_outputs(&world, desktop, 1);
    fake_key(desktop, FAKE_KEY_M, WM_MOD_SUPER);
    fake_key(desktop, FAKE_KEY_M, WM_MOD_SUPER);
    wm_rect restored = fake_client_of(window);
    set_outputs(&world, desktop, 2);
    CHECK_RECT_EQ(restored, fake_client_of(window));
    wm_desktop_destroy(desktop);
}

static void test_tile_while_evacuated_stays(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    wm_window *window = fake_map_at(desktop, 1, on_second);
    set_outputs(&world, desktop, 1);
    fake_key(desktop, WM_KEYSYM_LEFT, WM_MOD_SUPER);
    wm_rect tiled = fake_client_of(window);
    CHECK(tiled.x < FAKE_SCREEN_WIDTH);
    set_outputs(&world, desktop, 2);
    CHECK_RECT_EQ(tiled, fake_client_of(window));
    wm_desktop_destroy(desktop);
}

static const check_case CASES[] = {
    {"windows_stay_while_no_output", test_windows_stay_while_no_output},
    {"window_returns_to_readded_output", test_window_returns_to_readded_output},
    {"client_resize_keeps_return", test_client_resize_keeps_return},
    {"moved_window_stays_after_output_returns", test_moved_window_stays_after_output_returns},
    {"maximized_window_returns_with_saved_geometry",
     test_maximized_window_returns_with_saved_geometry},
    {"output_back_at_other_size_drops_return", test_output_back_at_other_size_drops_return},
    {"maximize_while_evacuated_stays", test_maximize_while_evacuated_stays},
    {"tile_while_evacuated_stays", test_tile_while_evacuated_stays},
};

int main(void)
{
    return check_run(CASES, sizeof(CASES) / sizeof(CASES[0]));
}
