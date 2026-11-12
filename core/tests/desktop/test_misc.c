#include "check.h"
#include "fake_input.h"

#include <string.h>

static const wm_rect small = {0, 0, 400, 300};

static wm_frame_state title_state(const wm_window *window)
{
    return wm_window_current_view(window).decor->title_text.state;
}

static void test_zoom_steps_in_and_out(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    fake_click where = {WM_TARGET_DESKTOP, NULL, {300, 200}, 0, WM_MOD_ALT, -1};
    CHECK_INT_EQ(WM_CONSUMED, fake_scroll(&world, &where, -2));
    CHECK_DOUBLE_NEAR(65536.0 / (65536.0 - 8192.0), world.zoom.level, 1e-9);
    CHECK_INT_EQ(300, world.zoom.center.x);
    CHECK(world.zoom.zoom_pointer);
    fake_click moved = {WM_TARGET_DESKTOP, NULL, {500, 600}, 0, 0, -1};
    fake_motion(&world, desktop, &moved);
    CHECK_INT_EQ(500, world.zoom.center.x);
    fake_scroll(&world, &where, 2);
    CHECK_DOUBLE_NEAR(1.0, world.zoom.level, 1e-9);
    wm_desktop_destroy(desktop);
}

static void test_zoom_is_clamped(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    fake_click where = {WM_TARGET_DESKTOP, NULL, {300, 200}, 0, WM_MOD_ALT, -1};
    fake_scroll(&world, &where, -40);
    CHECK_DOUBLE_NEAR(64.0, world.zoom.level, 1e-9);
    wm_desktop_destroy(desktop);
}

static void test_zoom_follows_tracked_pointer(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    fake_click where = {WM_TARGET_DESKTOP, NULL, {300, 200}, 0, WM_MOD_ALT, -1};
    fake_scroll(&world, &where, -2);
    wm_pointer_track(desktop, (wm_point) {500, 600});
    CHECK_INT_EQ(500, world.zoom.center.x);
    CHECK_INT_EQ(600, world.zoom.center.y);
    wm_desktop_destroy(desktop);
}

static void test_zoom_consumes_partial_scroll(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    fake_click where = {WM_TARGET_CLIENT, window, {100, 100}, 0, WM_MOD_ALT, -1};
    CHECK_INT_EQ(WM_CONSUMED, fake_scroll(&world, &where, 0));
    CHECK_INT_EQ(0, world.zoom_calls);
    where.modifiers = 0;
    CHECK_INT_EQ(WM_FORWARD, fake_scroll(&world, &where, 0));
    wm_desktop_destroy(desktop);
}

static void test_urgent_blink_stops_looking_active(void)
{
    fake_world world;
    wm_config config = fake_config();
    config.focus.urgent_blink = true;
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    fake_map(desktop, 2, small);
    wm_window_demand_attention(desktop, window);
    CHECK(wm_window_current_view(window).urgent);
    CHECK_INT_EQ(WM_FRAME_INACTIVE, title_state(window));
    fake_advance(&world, desktop, 500);
    CHECK_INT_EQ(WM_FRAME_ACTIVE, title_state(window));
    fake_advance(&world, desktop, 500);
    CHECK_INT_EQ(WM_FRAME_INACTIVE, title_state(window));
    fake_advance(&world, desktop, 5000);
    CHECK_INT_EQ(WM_FRAME_ACTIVE, title_state(window));
    CHECK(!world.timer_pending);
    wm_desktop_destroy(desktop);
}

static void test_focus_clears_urgency(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    fake_map(desktop, 2, small);
    wm_window_demand_attention(desktop, window);
    fake_click click = {WM_TARGET_CLIENT, window, {900, 500}, WM_MOUSE_LEFT, 0, -1};
    fake_press(&world, desktop, &click);
    CHECK(!wm_window_current_view(window).urgent);
    wm_desktop_destroy(desktop);
}

static void test_same_width_title_change_redraws_frame(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    wm_window_set_title(desktop, window, "abc");
    unsigned before = wm_window_current_view(window).decor_serial;
    wm_window_set_title(desktop, window, "xyz");
    CHECK(wm_window_current_view(window).decor_serial != before);
    wm_desktop_destroy(desktop);
}

static void test_not_responding_after_close_asks_terminate(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    wm_window_not_responding(desktop, window);
    CHECK(world.terminate_asked == NULL);
    fake_key(desktop, FAKE_KEY_F4, WM_MOD_ALT);
    CHECK(world.closed == fake_handle(1));
    wm_window_not_responding(desktop, window);
    CHECK(world.terminate_asked == fake_handle(1));
    wm_desktop_destroy(desktop);
}

static void test_close_without_focus_logs_out(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    CHECK_INT_EQ(WM_CONSUMED, fake_key(desktop, FAKE_KEY_F4, WM_MOD_ALT));
    CHECK_INT_EQ(1, world.logout_calls);
    wm_desktop_destroy(desktop);
}

static void test_command_binding_spawns(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    fake_key(desktop, FAKE_KEY_T, WM_MOD_CTRL | WM_MOD_ALT);
    CHECK(world.spawned[0] == 'x');
    CHECK_INT_EQ(WM_FORWARD, fake_key(desktop, FAKE_KEY_T, WM_MOD_CTRL));
    wm_desktop_destroy(desktop);
}

static void test_lone_modifier_spawns_on_release(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    CHECK_INT_EQ(WM_FORWARD, fake_key(desktop, FAKE_KEY_SUPER_L, 0));
    CHECK(world.spawned[0] == '\0');
    CHECK_INT_EQ(WM_FORWARD, fake_key_release(desktop, FAKE_KEY_SUPER_L, WM_MOD_SUPER));
    CHECK(strcmp("menu", world.spawned) == 0);
    wm_desktop_destroy(desktop);
}

static void test_modifier_used_in_chord_does_not_spawn(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    fake_key(desktop, FAKE_KEY_SUPER_L, 0);
    fake_key(desktop, FAKE_KEY_M, WM_MOD_SUPER);
    fake_key_release(desktop, FAKE_KEY_SUPER_L, WM_MOD_SUPER);
    CHECK(world.spawned[0] == '\0');
    fake_key(desktop, FAKE_KEY_SUPER_L, 0);
    fake_click click = {WM_TARGET_DESKTOP, NULL, {800, 500}, WM_MOUSE_LEFT, 0, -1};
    fake_press(&world, desktop, &click);
    fake_key_release(desktop, FAKE_KEY_SUPER_L, WM_MOD_SUPER);
    CHECK(world.spawned[0] == '\0');
    fake_key(desktop, FAKE_KEY_SUPER_L, 0);
    fake_scroll(&world, &click, 1);
    fake_key_release(desktop, FAKE_KEY_SUPER_L, WM_MOD_SUPER);
    CHECK(world.spawned[0] == '\0');
    wm_desktop_destroy(desktop);
}

static void test_release_of_earlier_key_keeps_tap(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    fake_key(desktop, FAKE_KEY_A, 0);
    fake_key(desktop, FAKE_KEY_SUPER_L, 0);
    fake_key_release(desktop, FAKE_KEY_A, WM_MOD_SUPER);
    fake_key_release(desktop, FAKE_KEY_SUPER_L, WM_MOD_SUPER);
    CHECK(strcmp("menu", world.spawned) == 0);
    wm_desktop_destroy(desktop);
}

static void test_app_shortcut_reports_press_and_release(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    CHECK_INT_EQ(WM_CONSUMED, fake_key(desktop, FAKE_KEY_T, WM_MOD_SUPER));
    CHECK_INT_EQ(FAKE_APP_SHORTCUT, world.app_shortcut_id);
    CHECK_INT_EQ(1, world.app_shortcut_presses);
    CHECK_INT_EQ(WM_CONSUMED, fake_key_release(desktop, FAKE_KEY_T, WM_MOD_SUPER));
    CHECK_INT_EQ(1, world.app_shortcut_releases);
    CHECK_INT_EQ(WM_FORWARD, fake_key_release(desktop, FAKE_KEY_T, WM_MOD_SUPER));
    wm_desktop_destroy(desktop);
}

static void test_second_app_shortcut_releases_the_first(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    fake_key(desktop, FAKE_KEY_T, WM_MOD_SUPER);
    fake_key(desktop, FAKE_KEY_T, WM_MOD_SUPER);
    CHECK_INT_EQ(2, world.app_shortcut_presses);
    CHECK_INT_EQ(1, world.app_shortcut_releases);
    wm_desktop_destroy(desktop);
}

static void test_key_is_bound_checks_every_binding_kind(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    CHECK(wm_key_is_bound(desktop, &(wm_key_event) {FAKE_KEY_F4, WM_MOD_ALT, 0}));
    CHECK(wm_key_is_bound(desktop, &(wm_key_event) {FAKE_KEY_T, WM_MOD_CTRL | WM_MOD_ALT, 0}));
    CHECK(wm_key_is_bound(desktop, &(wm_key_event) {FAKE_KEY_T, WM_MOD_SUPER, 0}));
    CHECK(!wm_key_is_bound(desktop, &(wm_key_event) {FAKE_KEY_T, WM_MOD_CTRL, 0}));
    CHECK_INT_EQ(0, world.app_shortcut_presses);
    wm_desktop_destroy(desktop);
}

static void test_user_shortcut_beats_app_shortcut(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    fake_key(desktop, FAKE_KEY_T, WM_MOD_CTRL | WM_MOD_ALT);
    CHECK(world.spawned[0] == 'x');
    CHECK_INT_EQ(0, world.app_shortcut_presses);
    wm_desktop_destroy(desktop);
}

static void test_fill_key_grows_to_neighbours(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    fake_map_at(desktop, 1, (wm_rect) {100, 100, 200, 200});
    wm_window *window = fake_map_at(desktop, 2, (wm_rect) {500, 120, 200, 200});
    fake_click click = {WM_TARGET_CLIENT, window, {600, 200}, WM_MOUSE_LEFT, 0, -1};
    fake_press(&world, desktop, &click);
    fake_key(desktop, FAKE_KEY_F, WM_MOD_SUPER);
    CHECK_RECT_EQ(((wm_rect) {308, 20, 1608, 1056}), wm_window_current_view(window).client);
    wm_desktop_destroy(desktop);
}

static void test_fill_key_keeps_max_size(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_size_hints hints = {.min = {100, 100}, .max = {600, 500}};
    wm_window *window = fake_map_with_hints(desktop, small, &hints);
    fake_key(desktop, FAKE_KEY_F, WM_MOD_SUPER);
    CHECK_INT_EQ(600, fake_client_of(window).width);
    CHECK_INT_EQ(500, fake_client_of(window).height);
    wm_desktop_destroy(desktop);
}

static void test_request_configure_ignored_when_maximized(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    wm_rect wanted = {100, 100, 500, 400};
    wm_request_configure(desktop, window, &wanted);
    CHECK_RECT_EQ(wanted, wm_window_current_view(window).client);
    wm_request_state(desktop, window, &(wm_state_change) {WM_STATE_MAXIMIZED, 0});
    wm_request_configure(desktop, window, &wanted);
    CHECK(wm_window_current_view(window).maximized);
    CHECK_INT_EQ(1920, wm_window_current_view(window).client.width);
    wm_desktop_destroy(desktop);
}

static void test_timer_is_scheduled_for_focus_delay(void)
{
    fake_world world;
    wm_config config = fake_config();
    config.focus.click_to_focus = false;
    config.focus.focus_delay_ms = 250;
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    fake_map(desktop, 2, small);
    fake_click where = {WM_TARGET_CLIENT, window, {10, 10}, 0, 0, -1};
    fake_motion(&world, desktop, &where);
    CHECK(world.timer_pending);
    CHECK_INT_EQ(world.now_ms + 250, world.timer_deadline_ms);
    wm_desktop_destroy(desktop);
}

static const check_case CASES[] = {
    {"zoom_steps_in_and_out", test_zoom_steps_in_and_out},
    {"zoom_is_clamped", test_zoom_is_clamped},
    {"zoom_consumes_partial_scroll", test_zoom_consumes_partial_scroll},
    {"zoom_follows_tracked_pointer", test_zoom_follows_tracked_pointer},
    {"urgent_blink_stops_looking_active", test_urgent_blink_stops_looking_active},
    {"focus_clears_urgency", test_focus_clears_urgency},
    {"same_width_title_change_redraws_frame", test_same_width_title_change_redraws_frame},
    {"not_responding_after_close_asks_terminate",
     test_not_responding_after_close_asks_terminate},
    {"close_without_focus_logs_out", test_close_without_focus_logs_out},
    {"command_binding_spawns", test_command_binding_spawns},
    {"lone_modifier_spawns_on_release", test_lone_modifier_spawns_on_release},
    {"modifier_used_in_chord_does_not_spawn", test_modifier_used_in_chord_does_not_spawn},
    {"release_of_earlier_key_keeps_tap", test_release_of_earlier_key_keeps_tap},
    {"app_shortcut_reports_press_and_release", test_app_shortcut_reports_press_and_release},
    {"second_app_shortcut_releases_the_first", test_second_app_shortcut_releases_the_first},
    {"key_is_bound_checks_every_binding_kind", test_key_is_bound_checks_every_binding_kind},
    {"user_shortcut_beats_app_shortcut", test_user_shortcut_beats_app_shortcut},
    {"fill_key_grows_to_neighbours", test_fill_key_grows_to_neighbours},
    {"fill_key_keeps_max_size", test_fill_key_keeps_max_size},
    {"request_configure_ignored_when_maximized",
     test_request_configure_ignored_when_maximized},
    {"timer_is_scheduled_for_focus_delay", test_timer_is_scheduled_for_focus_delay},
};

int main(void)
{
    return check_run(CASES, sizeof(CASES) / sizeof(CASES[0]));
}
