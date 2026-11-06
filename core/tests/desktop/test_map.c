#include "check.h"
#include "fake_ports.h"

static void test_center_placement(void)
{
    fake_world world;
    fake_world_init(&world);
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_create(&world, &config);
    wm_window *window = fake_map(desktop, 1, (wm_rect) {0, 0, 400, 300});
    CHECK_RECT_EQ(((wm_rect) {760, 398, 400, 300}), fake_client_of(window));
    CHECK_RECT_EQ(((wm_rect) {760, 398, 400, 300}), fake_configured(&world, fake_handle(1)));
    wm_desktop_destroy(desktop);
}

static void test_mouse_placement(void)
{
    fake_world world;
    fake_world_init(&world);
    wm_config config = fake_config();
    config.placement.mode = WM_PLACE_MOUSE;
    world.pointer = (wm_point) {100, 100};
    wm_desktop *desktop = fake_desktop_create(&world, &config);
    wm_window *window = fake_map(desktop, 1, (wm_rect) {0, 0, 400, 300});
    CHECK_RECT_EQ(((wm_rect) {4, 20, 400, 300}), fake_client_of(window));
    wm_desktop_destroy(desktop);
}

static void test_smart_placement_avoids_overlap(void)
{
    fake_world world;
    fake_world_init(&world);
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_create(&world, &config);
    wm_window *first = fake_map(desktop, 1, (wm_rect) {0, 0, 1000, 700});
    wm_window *second = fake_map(desktop, 2, (wm_rect) {0, 0, 1000, 700});
    CHECK_RECT_EQ(((wm_rect) {4, 20, 1000, 700}), fake_client_of(first));
    CHECK_RECT_EQ(((wm_rect) {916, 376, 1000, 700}), fake_client_of(second));
    wm_desktop_destroy(desktop);
}

static void test_transient_centered_on_parent(void)
{
    fake_world world;
    fake_world_init(&world);
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_create(&world, &config);
    wm_window *parent = fake_map(desktop, 1, (wm_rect) {0, 0, 400, 300});
    wm_window_desc desc = fake_desc(2, (wm_rect) {0, 0, 200, 100});
    desc.type = WM_TYPE_DIALOG;
    desc.parent = parent;
    wm_window *dialog = wm_window_map(desktop, &desc);
    CHECK_RECT_EQ(((wm_rect) {860, 498, 200, 100}), fake_client_of(dialog));
    wm_desktop_destroy(desktop);
}

static void test_given_position_is_kept_visible(void)
{
    fake_world world;
    fake_world_init(&world);
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_create(&world, &config);
    wm_window_desc desc = fake_desc(1, (wm_rect) {1800, 50, 400, 300});
    desc.has_position = true;
    wm_window *window = wm_window_map(desktop, &desc);
    CHECK_RECT_EQ(((wm_rect) {1516, 50, 400, 300}), fake_client_of(window));
    wm_desktop_destroy(desktop);
}

static void test_large_window_is_auto_maximized(void)
{
    fake_world world;
    fake_world_init(&world);
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_create(&world, &config);
    wm_window *window = fake_map(desktop, 1, (wm_rect) {0, 0, 1920, 1080});
    wm_window_view view = wm_window_current_view(window);
    CHECK(view.maximized);
    CHECK_RECT_EQ(((wm_rect) {0, 0, 1920, 1080}), view.frame);
    wm_desktop_destroy(desktop);
}

static void test_margins_reduce_work_area(void)
{
    fake_world world;
    fake_world_init(&world);
    wm_config config = fake_config();
    config.placement.margins = (wm_margins) {100, 0, 0, 0};
    world.outputs[0].usable = (wm_rect) {0, 0, 1920, 1050};
    wm_desktop *desktop = fake_desktop_create(&world, &config);
    wm_window *window = fake_map(desktop, 1, (wm_rect) {0, 0, 400, 300});
    CHECK_RECT_EQ(((wm_rect) {760, 433, 400, 300}), fake_client_of(window));
    wm_desktop_destroy(desktop);
}

static void test_transient_follows_parent_workspace(void)
{
    fake_world world;
    fake_world_init(&world);
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_create(&world, &config);
    wm_window *parent = fake_map(desktop, 1, (wm_rect) {0, 0, 400, 300});
    wm_desktop_switch_workspace(desktop, 2);
    wm_window_desc desc = fake_desc(2, (wm_rect) {0, 0, 200, 100});
    desc.type = WM_TYPE_DIALOG;
    desc.parent = parent;
    wm_window *dialog = wm_window_map(desktop, &desc);
    CHECK_INT_EQ(0, wm_window_current_view(dialog).workspace);
    CHECK(!wm_window_current_view(dialog).visible);
    wm_desktop_destroy(desktop);
}

static void test_unmap_reports_stacking(void)
{
    fake_world world;
    fake_world_init(&world);
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_create(&world, &config);
    wm_window *first = fake_map(desktop, 1, (wm_rect) {0, 0, 400, 300});
    fake_map(desktop, 2, (wm_rect) {0, 0, 400, 300});
    wm_window_unmap(desktop, first);
    CHECK_INT_EQ(1, world.stacking_count);
    CHECK(world.stacking[0] == fake_handle(2));
    wm_desktop_destroy(desktop);
}

static void test_size_commit_keeps_right_edge_on_left_resize(void)
{
    fake_world world;
    fake_world_init(&world);
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_create(&world, &config);
    wm_window *window = fake_map(desktop, 1, (wm_rect) {0, 0, 400, 300});
    wm_request_resize(desktop, window, WM_EDGE_LEFT);
    wm_window_size_committed(desktop, window, (wm_size) {380, 300});
    CHECK_RECT_EQ(((wm_rect) {780, 398, 380, 300}), fake_client_of(window));
    wm_desktop_destroy(desktop);
}

static void test_launch_shows_busy_until_timeout(void)
{
    fake_world world;
    fake_world_init(&world);
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_create(&world, &config);
    wm_launch_started(desktop, "token-a");
    CHECK(!world.busy);
    fake_advance(&world, desktop, 150);
    CHECK(world.busy);
    CHECK_INT_EQ(WM_CURSOR_BUSY, world.cursor);
    fake_advance(&world, desktop, 9850);
    CHECK(!world.busy);
    CHECK_INT_EQ(WM_CURSOR_DEFAULT, world.cursor);
    wm_desktop_destroy(desktop);
}

static void test_launched_window_opens_on_launch_workspace(void)
{
    fake_world world;
    fake_world_init(&world);
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_create(&world, &config);
    wm_launch_started(desktop, "token-a");
    fake_advance(&world, desktop, 200);
    wm_desktop_switch_workspace(desktop, 2);
    wm_window_desc desc = fake_desc(1, (wm_rect) {0, 0, 400, 300});
    desc.launch_id = "token-a";
    wm_window *window = wm_window_map(desktop, &desc);
    CHECK_INT_EQ(0, wm_window_current_view(window).workspace);
    CHECK(!world.busy);
    wm_desktop_destroy(desktop);
}

static void test_finished_launch_stops_busy(void)
{
    fake_world world;
    fake_world_init(&world);
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_create(&world, &config);
    wm_launch_started(desktop, "token-a");
    wm_launch_started(desktop, "token-b");
    fake_advance(&world, desktop, 200);
    wm_launch_finished(desktop, "token-a");
    CHECK(world.busy);
    wm_launch_finished(desktop, "token-b");
    CHECK(!world.busy);
    wm_desktop_destroy(desktop);
}

static void test_finished_launch_still_places_window(void)
{
    fake_world world;
    fake_world_init(&world);
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_create(&world, &config);
    wm_launch_started(desktop, "token-a");
    fake_advance(&world, desktop, 200);
    wm_desktop_switch_workspace(desktop, 2);
    wm_launch_finished(desktop, "token-a");
    CHECK(!world.busy);
    wm_window_desc desc = fake_desc(1, (wm_rect) {0, 0, 400, 300});
    desc.launch_id = "token-a";
    fake_advance(&world, desktop, 500);
    wm_window *window = wm_window_map(desktop, &desc);
    CHECK_INT_EQ(0, wm_window_current_view(window).workspace);
    wm_desktop_destroy(desktop);
}

static void test_launch_follows_removed_workspace(void)
{
    fake_world world;
    fake_world_init(&world);
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_create(&world, &config);
    wm_desktop_switch_workspace(desktop, 3);
    wm_launch_started(desktop, "token-a");
    wm_desktop_remove_workspace(desktop, 1);
    wm_window_desc desc = fake_desc(1, (wm_rect) {0, 0, 400, 300});
    desc.launch_id = "token-a";
    wm_window *window = wm_window_map(desktop, &desc);
    CHECK_INT_EQ(2, wm_window_current_view(window).workspace);
    wm_desktop_destroy(desktop);
}

static void test_late_launch_id_moves_new_window(void)
{
    fake_world world;
    fake_world_init(&world);
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_create(&world, &config);
    wm_launch_started(desktop, "token-a");
    fake_advance(&world, desktop, 200);
    wm_desktop_switch_workspace(desktop, 1);
    wm_window *window = fake_map(desktop, 1, (wm_rect) {0, 0, 400, 300});
    CHECK_INT_EQ(1, wm_window_current_view(window).workspace);
    CHECK(world.busy);
    CHECK(wm_window_launched(desktop, window, "token-a"));
    CHECK_INT_EQ(0, wm_window_current_view(window).workspace);
    CHECK(!world.busy);
    wm_desktop_destroy(desktop);
}

static void test_old_window_keeps_its_workspace(void)
{
    fake_world world;
    fake_world_init(&world);
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_create(&world, &config);
    wm_desktop_switch_workspace(desktop, 1);
    wm_window *window = fake_map(desktop, 1, (wm_rect) {0, 0, 400, 300});
    fake_advance(&world, desktop, 5000);
    wm_desktop_switch_workspace(desktop, 0);
    wm_launch_started(desktop, "token-a");
    wm_window_launched(desktop, window, "token-a");
    CHECK_INT_EQ(1, wm_window_current_view(window).workspace);
    wm_desktop_destroy(desktop);
}

int main(void)
{
    static const check_case cases[] = {
        {"center_placement", test_center_placement},
        {"mouse_placement", test_mouse_placement},
        {"smart_placement_avoids_overlap", test_smart_placement_avoids_overlap},
        {"transient_centered_on_parent", test_transient_centered_on_parent},
        {"given_position_is_kept_visible", test_given_position_is_kept_visible},
        {"large_window_is_auto_maximized", test_large_window_is_auto_maximized},
        {"margins_reduce_work_area", test_margins_reduce_work_area},
        {"transient_follows_parent_workspace", test_transient_follows_parent_workspace},
        {"unmap_reports_stacking", test_unmap_reports_stacking},
        {"launch_shows_busy_until_timeout", test_launch_shows_busy_until_timeout},
        {"launched_window_opens_on_launch_workspace",
         test_launched_window_opens_on_launch_workspace},
        {"finished_launch_stops_busy", test_finished_launch_stops_busy},
        {"finished_launch_still_places_window", test_finished_launch_still_places_window},
        {"launch_follows_removed_workspace", test_launch_follows_removed_workspace},
        {"late_launch_id_moves_new_window", test_late_launch_id_moves_new_window},
        {"old_window_keeps_its_workspace", test_old_window_keeps_its_workspace},
        {"size_commit_keeps_right_edge_on_left_resize",
         test_size_commit_keeps_right_edge_on_left_resize},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
