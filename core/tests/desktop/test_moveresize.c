#include "check.h"
#include "fake_input.h"

static const wm_rect small = {0, 0, 400, 300};

static fake_click drag_title(fake_world *world, wm_desktop *desktop, wm_window *window)
{
    fake_click click = fake_on_part(window, WM_PART_TITLE, WM_MOUSE_LEFT);
    fake_press(world, desktop, &click);
    return click;
}

static void drag_to(fake_world *world, wm_desktop *desktop, fake_click *click, wm_point point)
{
    click->point = point;
    fake_motion(world, desktop, click);
}

static void test_keyboard_move_and_cancel(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    fake_key(desktop, FAKE_KEY_F7, WM_MOD_ALT);
    CHECK(wm_window_current_view(window).moving);
    CHECK_INT_EQ(960, world.warp.x);
    fake_key(desktop, WM_KEYSYM_RIGHT, 0);
    CHECK_INT_EQ(976, world.warp.x);
    CHECK_INT_EQ(776, fake_client_of(window).x);
    fake_key(desktop, WM_KEYSYM_ESCAPE, 0);
    CHECK_RECT_EQ(((wm_rect) {760, 398, 400, 300}), fake_client_of(window));
    CHECK(!wm_window_current_view(window).moving);
    wm_desktop_destroy(desktop);
}

static void test_keyboard_move_ends_on_return(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    fake_key(desktop, FAKE_KEY_F7, WM_MOD_ALT);
    fake_key(desktop, WM_KEYSYM_DOWN, 0);
    fake_key(desktop, WM_KEYSYM_RETURN, 0);
    CHECK_INT_EQ(414, fake_client_of(window).y);
    CHECK_INT_EQ(414, fake_configured(&world, fake_handle(1)).y);
    CHECK_INT_EQ(WM_FORWARD, fake_key(desktop, WM_KEYSYM_DOWN, 0));
    wm_desktop_destroy(desktop);
}

static void test_drag_snaps_to_screen_border(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    fake_click click = drag_title(&world, desktop, window);
    wm_point start = click.point;
    drag_to(&world, desktop, &click, (wm_point) {start.x - 750, start.y});
    CHECK_INT_EQ(4, fake_client_of(window).x);
    CHECK_INT_EQ(WM_CURSOR_MOVE, world.cursor);
    fake_release(&world, desktop, &click);
    CHECK(!wm_window_current_view(window).moving);
    CHECK_INT_EQ(WM_CURSOR_DEFAULT, world.cursor);
    wm_desktop_destroy(desktop);
}

static void test_tile_left_key_toggles(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    fake_key(desktop, WM_KEYSYM_LEFT, WM_MOD_SUPER);
    CHECK_RECT_EQ(((wm_rect) {4, 20, 952, 1056}), fake_client_of(window));
    CHECK_INT_EQ(WM_EDGE_LEFT | WM_EDGE_TOP | WM_EDGE_BOTTOM,
                 wm_window_current_view(window).tiled_edges);
    fake_key(desktop, WM_KEYSYM_LEFT, WM_MOD_SUPER);
    CHECK_RECT_EQ(((wm_rect) {760, 398, 400, 300}), fake_client_of(window));
    wm_desktop_destroy(desktop);
}

static void test_drag_to_left_edge_tiles(void)
{
    fake_world world;
    wm_config config = fake_config();
    config.move.wrap_windows = false;
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    fake_click click = drag_title(&world, desktop, window);
    drag_to(&world, desktop, &click, (wm_point) {1, 540});
    fake_release(&world, desktop, &click);
    CHECK_RECT_EQ(((wm_rect) {4, 20, 952, 1056}), fake_client_of(window));
    drag_title(&world, desktop, window);
    drag_to(&world, desktop, &click, (wm_point) {600, 540});
    fake_release(&world, desktop, &click);
    CHECK_INT_EQ(400, fake_client_of(window).width);
    CHECK_INT_EQ(0, wm_window_current_view(window).tiled_edges);
    wm_desktop_destroy(desktop);
}

static void test_drag_maximized_restores_size(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    fake_key(desktop, FAKE_KEY_M, WM_MOD_SUPER);
    fake_click click = drag_title(&world, desktop, window);
    drag_to(&world, desktop, &click, (wm_point) {click.point.x + 10, click.point.y + 300});
    fake_release(&world, desktop, &click);
    wm_window_view view = wm_window_current_view(window);
    CHECK(!view.maximized);
    CHECK_INT_EQ(400, view.client.width);
    CHECK_INT_EQ(300, view.client.height);
    CHECK_INT_EQ(770, view.client.x);
    wm_desktop_destroy(desktop);
}

static void test_box_move_shows_outline(void)
{
    fake_world world;
    wm_config config = fake_config();
    config.move.box_move = true;
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    fake_click click = drag_title(&world, desktop, window);
    drag_to(&world, desktop, &click, (wm_point) {click.point.x + 100, click.point.y + 50});
    CHECK(world.outline_visible);
    CHECK_RECT_EQ(((wm_rect) {856, 428, 408, 324}), world.outline);
    CHECK_RECT_EQ(((wm_rect) {760, 398, 400, 300}), fake_client_of(window));
    fake_release(&world, desktop, &click);
    CHECK(!world.outline_visible);
    CHECK_RECT_EQ(((wm_rect) {860, 448, 400, 300}), fake_client_of(window));
    wm_desktop_destroy(desktop);
}

static void test_resize_from_right_edge(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    fake_click click = fake_on_part(window, WM_PART_SIDE_RIGHT, WM_MOUSE_LEFT);
    fake_press(&world, desktop, &click);
    drag_to(&world, desktop, &click, (wm_point) {click.point.x + 50, click.point.y});
    CHECK_INT_EQ(WM_CURSOR_RESIZE_RIGHT, world.cursor);
    fake_release(&world, desktop, &click);
    CHECK_RECT_EQ(((wm_rect) {760, 398, 450, 300}), fake_configured(&world, fake_handle(1)));
    wm_desktop_destroy(desktop);
}

static void test_frame_follows_drawn_size_during_resize(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    fake_click click = fake_on_part(window, WM_PART_SIDE_LEFT, WM_MOUSE_LEFT);
    fake_press(&world, desktop, &click);
    drag_to(&world, desktop, &click, (wm_point) {click.point.x - 50, click.point.y});
    CHECK_RECT_EQ(((wm_rect) {760, 398, 400, 300}), wm_window_current_view(window).shown_client);
    CHECK_RECT_EQ(((wm_rect) {710, 398, 450, 300}), wm_window_current_view(window).client);
    fake_release(&world, desktop, &click);
    CHECK_RECT_EQ(((wm_rect) {760, 398, 400, 300}), wm_window_current_view(window).shown_client);
    wm_window_content_drawn(desktop, window, (wm_size) {450, 300});
    CHECK_RECT_EQ(((wm_rect) {710, 398, 450, 300}), wm_window_current_view(window).shown_client);
    wm_desktop_destroy(desktop);
}

static void test_frame_follows_drawn_size_from_top_edge(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    fake_click click = fake_on_part(window, WM_PART_SIDE_TOP, WM_MOUSE_LEFT);
    fake_press(&world, desktop, &click);
    drag_to(&world, desktop, &click, (wm_point) {click.point.x, click.point.y - 40});
    wm_window_view view = wm_window_current_view(window);
    CHECK_INT_EQ(view.client.y + view.client.height, view.shown_client.y + 300);
    CHECK_INT_EQ(300, view.shown_client.height);
    fake_release(&world, desktop, &click);
    wm_desktop_destroy(desktop);
}

static void test_resize_respects_minimum_size(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window_desc desc = fake_desc(1, small);
    desc.size_hints.min = (wm_size) {380, 280};
    wm_window *window = wm_window_map(desktop, &desc);
    fake_key(desktop, FAKE_KEY_F8, WM_MOD_ALT);
    fake_key(desktop, WM_KEYSYM_RIGHT, 0);
    fake_key(desktop, WM_KEYSYM_LEFT, 0);
    fake_key(desktop, WM_KEYSYM_LEFT, 0);
    fake_key(desktop, WM_KEYSYM_LEFT, 0);
    fake_key(desktop, WM_KEYSYM_RETURN, 0);
    CHECK_INT_EQ(380, fake_client_of(window).width);
    wm_desktop_destroy(desktop);
}

static wm_window *map_with_steps(wm_desktop *desktop, wm_rect geometry, wm_size base,
                                 wm_size step)
{
    wm_window_desc desc = fake_desc(1, geometry);
    desc.size_hints.base = base;
    desc.size_hints.step = step;
    return wm_window_map(desktop, &desc);
}

static void test_resize_snaps_to_steps(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = map_with_steps(desktop, (wm_rect) {0, 0, 404, 304}, (wm_size) {4, 4},
                                       (wm_size) {10, 20});
    fake_click click = fake_on_part(window, WM_PART_SIDE_RIGHT, WM_MOUSE_LEFT);
    fake_press(&world, desktop, &click);
    drag_to(&world, desktop, &click, (wm_point) {click.point.x + 57, click.point.y});
    CHECK_INT_EQ(454, fake_client_of(window).width);
    CHECK(world.size_visible);
    CHECK_INT_EQ(45, world.size.columns);
    CHECK_INT_EQ(15, world.size.rows);
    fake_release(&world, desktop, &click);
    CHECK(!world.size_visible);
    wm_desktop_destroy(desktop);
}

static void test_app_resize_ignores_steps(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = map_with_steps(desktop, (wm_rect) {0, 0, 404, 304}, (wm_size) {4, 4},
                                       (wm_size) {10, 20});
    wm_rect wanted = {100, 100, 461, 311};
    wm_request_configure(desktop, window, &wanted);
    CHECK_INT_EQ(461, fake_client_of(window).width);
    CHECK_INT_EQ(311, fake_client_of(window).height);
    wm_desktop_destroy(desktop);
}

static void test_keyboard_resize_uses_steps(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = map_with_steps(desktop, (wm_rect) {0, 0, 300, 300}, (wm_size) {0, 0},
                                       (wm_size) {3, 25});
    fake_key(desktop, FAKE_KEY_F8, WM_MOD_ALT);
    CHECK(world.size_visible);
    CHECK_INT_EQ(100, world.size.columns);
    CHECK_INT_EQ(12, world.size.rows);
    fake_key(desktop, WM_KEYSYM_RIGHT, 0);
    fake_key(desktop, WM_KEYSYM_RIGHT, 0);
    fake_key(desktop, WM_KEYSYM_DOWN, 0);
    fake_key(desktop, WM_KEYSYM_DOWN, 0);
    fake_key(desktop, WM_KEYSYM_RETURN, 0);
    CHECK_INT_EQ(309, fake_client_of(window).width);
    CHECK_INT_EQ(325, fake_client_of(window).height);
    CHECK(!world.size_visible);
    wm_desktop_destroy(desktop);
}

static void test_resize_keeps_aspect_ratio(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window_desc desc = fake_desc(1, (wm_rect) {0, 0, 400, 200});
    desc.size_hints.min_aspect = (wm_size) {2, 1};
    desc.size_hints.max_aspect = (wm_size) {2, 1};
    wm_window *window = wm_window_map(desktop, &desc);
    fake_click click = fake_on_part(window, WM_PART_SIDE_RIGHT, WM_MOUSE_LEFT);
    fake_press(&world, desktop, &click);
    drag_to(&world, desktop, &click, (wm_point) {click.point.x + 100, click.point.y});
    fake_release(&world, desktop, &click);
    CHECK_INT_EQ(500, fake_client_of(window).width);
    CHECK_INT_EQ(250, fake_client_of(window).height);
    CHECK(!world.size_visible);
    wm_desktop_destroy(desktop);
}

static void test_resize_from_bottom_widens_for_ratio(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window_desc desc = fake_desc(1, (wm_rect) {0, 0, 400, 200});
    desc.size_hints.min_aspect = (wm_size) {2, 1};
    desc.size_hints.max_aspect = (wm_size) {2, 1};
    wm_window *window = wm_window_map(desktop, &desc);
    fake_click click = fake_on_part(window, WM_PART_SIDE_BOTTOM, WM_MOUSE_LEFT);
    fake_press(&world, desktop, &click);
    drag_to(&world, desktop, &click, (wm_point) {click.point.x, click.point.y + 100});
    fake_release(&world, desktop, &click);
    CHECK_INT_EQ(600, fake_client_of(window).width);
    CHECK_INT_EQ(300, fake_client_of(window).height);
    wm_desktop_destroy(desktop);
}

static void test_easy_click_moves_from_content(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    fake_click click = {WM_TARGET_CLIENT, window, {900, 500}, WM_MOUSE_LEFT, WM_MOD_ALT};
    CHECK_INT_EQ(WM_CONSUMED, fake_press(&world, desktop, &click));
    drag_to(&world, desktop, &click, (wm_point) {950, 520});
    fake_release(&world, desktop, &click);
    CHECK_RECT_EQ(((wm_rect) {810, 418, 400, 300}), fake_client_of(window));
    wm_desktop_destroy(desktop);
}

static void test_drag_wraps_to_next_workspace(void)
{
    fake_world world;
    wm_config config = fake_config();
    config.move.wrap_resistance = 2;
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    fake_click click = drag_title(&world, desktop, window);
    for (int i = 0; i < 4; i++) {
        drag_to(&world, desktop, &click, (wm_point) {0, 540});
    }
    CHECK_INT_EQ(3, world.workspaces.current);
    CHECK_INT_EQ(3, wm_window_current_view(window).workspace);
    CHECK(world.warp.x > 1000);
    fake_release(&world, desktop, &click);
    wm_desktop_destroy(desktop);
}

static void test_drag_wraps_to_the_row_above(void)
{
    fake_world world;
    wm_config config = fake_config();
    config.move.wrap_resistance = 2;
    config.workspaces.rows = 2;
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    fake_click click = drag_title(&world, desktop, window);
    for (int i = 0; i < 4; i++) {
        drag_to(&world, desktop, &click, (wm_point) {click.point.x, 0});
    }
    CHECK_INT_EQ(world.workspaces.count / 2, world.workspaces.current);
    CHECK_INT_EQ(world.workspaces.count / 2, wm_window_current_view(window).workspace);
    CHECK(world.warp.y > 500);
    fake_release(&world, desktop, &click);
    wm_desktop_destroy(desktop);
}

static void test_closing_last_moved_window_frees_pointer(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *moved = fake_map(desktop, 1, small);
    wm_window *other = fake_map(desktop, 2, (wm_rect) {1200, 600, 400, 300});
    fake_click click = drag_title(&world, desktop, moved);
    drag_to(&world, desktop, &click, (wm_point) {click.point.x + 40, click.point.y});
    fake_release(&world, desktop, &click);
    wm_window_unmap(desktop, moved);
    fake_click hover = {WM_TARGET_CLIENT, other, {1300, 700}, 0, 0};
    CHECK_INT_EQ(WM_FORWARD, fake_motion(&world, desktop, &hover));
    wm_desktop_destroy(desktop);
}

int main(void)
{
    static const check_case cases[] = {
        {"keyboard_move_and_cancel", test_keyboard_move_and_cancel},
        {"keyboard_move_ends_on_return", test_keyboard_move_ends_on_return},
        {"drag_snaps_to_screen_border", test_drag_snaps_to_screen_border},
        {"tile_left_key_toggles", test_tile_left_key_toggles},
        {"drag_to_left_edge_tiles", test_drag_to_left_edge_tiles},
        {"drag_maximized_restores_size", test_drag_maximized_restores_size},
        {"box_move_shows_outline", test_box_move_shows_outline},
        {"resize_from_right_edge", test_resize_from_right_edge},
        {"frame_follows_drawn_size_during_resize", test_frame_follows_drawn_size_during_resize},
        {"frame_follows_drawn_size_from_top_edge", test_frame_follows_drawn_size_from_top_edge},
        {"resize_respects_minimum_size", test_resize_respects_minimum_size},
        {"resize_snaps_to_steps", test_resize_snaps_to_steps},
        {"app_resize_ignores_steps", test_app_resize_ignores_steps},
        {"keyboard_resize_uses_steps", test_keyboard_resize_uses_steps},
        {"resize_keeps_aspect_ratio", test_resize_keeps_aspect_ratio},
        {"resize_from_bottom_widens_for_ratio", test_resize_from_bottom_widens_for_ratio},
        {"easy_click_moves_from_content", test_easy_click_moves_from_content},
        {"drag_wraps_to_next_workspace", test_drag_wraps_to_next_workspace},
        {"drag_wraps_to_the_row_above", test_drag_wraps_to_the_row_above},
        {"closing_last_moved_window_frees_pointer",
         test_closing_last_moved_window_frees_pointer},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
