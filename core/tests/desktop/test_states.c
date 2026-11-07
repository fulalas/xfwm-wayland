#include "check.h"
#include "fake_input.h"

static const wm_rect small = {0, 0, 400, 300};

static wm_window_view view_of(const wm_window *window)
{
    return wm_window_current_view(window);
}

static void test_maximize_key_and_restore(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    fake_key(desktop, FAKE_KEY_M, WM_MOD_SUPER);
    CHECK(view_of(window).maximized);
    CHECK_RECT_EQ(((wm_rect) {0, 20, 1920, 1060}), fake_configured(&world, fake_handle(1)));
    CHECK_INT_EQ(0, view_of(window).decor->extents.left);
    fake_key(desktop, FAKE_KEY_M, WM_MOD_SUPER);
    CHECK(!view_of(window).maximized);
    CHECK_RECT_EQ(((wm_rect) {760, 398, 400, 300}), fake_configured(&world, fake_handle(1)));
    wm_desktop_destroy(desktop);
}

static void test_maximize_keeps_borders_without_borderless(void)
{
    fake_world world;
    wm_config config = fake_config();
    config.frame.borderless_maximize = false;
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    wm_request_state(desktop, window, &(wm_state_change) {WM_STATE_MAXIMIZED, 0});
    CHECK_RECT_EQ(((wm_rect) {4, 20, 1912, 1056}), view_of(window).client);
    CHECK_RECT_EQ(((wm_rect) {0, 0, 1920, 1080}), view_of(window).frame);
    wm_desktop_destroy(desktop);
}

static void test_vertical_maximize(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    wm_request_state(desktop, window, &(wm_state_change) {WM_STATE_MAXIMIZED_VERT, 0});
    CHECK_RECT_EQ(((wm_rect) {760, 20, 400, 1056}), view_of(window).client);
    CHECK(!view_of(window).maximized);
    CHECK_INT_EQ(WM_EDGE_TOP | WM_EDGE_BOTTOM, view_of(window).tiled_edges);
    wm_desktop_destroy(desktop);
}

static void test_fullscreen_covers_output_without_frame(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    wm_request_state(desktop, window, &(wm_state_change) {WM_STATE_FULLSCREEN, 0});
    CHECK_RECT_EQ(((wm_rect) {0, 0, 1920, 1080}), view_of(window).client);
    CHECK_RECT_EQ(((wm_rect) {0, 0, 1920, 1080}), view_of(window).frame);
    CHECK(!view_of(window).has_shadow);
    wm_request_state(desktop, window, &(wm_state_change) {0, WM_STATE_FULLSCREEN});
    CHECK_RECT_EQ(((wm_rect) {760, 398, 400, 300}), view_of(window).client);
    wm_desktop_destroy(desktop);
}

static void test_shade_keeps_only_title_and_bottom(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    wm_state_change change = {WM_STATE_SHADED, 0};
    wm_request_state(desktop, window, &change);
    CHECK(view_of(window).shaded);
    CHECK_INT_EQ(24, view_of(window).frame.height);
    wm_desktop_destroy(desktop);
}

static void test_minimize_hides_transients(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    fake_map(desktop, 3, small);
    wm_window *parent = fake_map(desktop, 1, small);
    wm_window_desc desc = fake_desc(2, (wm_rect) {0, 0, 200, 100});
    desc.type = WM_TYPE_DIALOG;
    desc.parent = parent;
    wm_window *dialog = wm_window_map(desktop, &desc);
    wm_request_state(desktop, parent, &(wm_state_change) {WM_STATE_MINIMIZED, 0});
    CHECK(view_of(parent).minimized);
    CHECK(!view_of(parent).visible);
    CHECK(!view_of(dialog).visible);
    CHECK(world.focused == fake_handle(3));
    wm_state_change change = {0, WM_STATE_MINIMIZED};
    wm_request_state(desktop, parent, &change);
    CHECK(view_of(parent).visible);
    CHECK(view_of(dialog).visible);
    wm_desktop_destroy(desktop);
}

static void test_sticky_window_visible_everywhere(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    wm_state_change change = {WM_STATE_STICKY, 0};
    wm_request_state(desktop, window, &change);
    wm_desktop_switch_workspace(desktop, 3);
    CHECK(view_of(window).visible);
    CHECK(view_of(window).sticky);
    CHECK_INT_EQ(3, view_of(window).workspace);
    wm_desktop_destroy(desktop);
}

static void test_inactive_opacity_and_shadow(void)
{
    fake_world world;
    wm_config config = fake_config();
    config.compositing.inactive_opacity_percent = 50;
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *first = fake_map(desktop, 1, small);
    fake_map(desktop, 2, small);
    wm_window_view view = view_of(first);
    CHECK_DOUBLE_NEAR(0.5, view.opacity.content, 1e-6);
    CHECK_DOUBLE_NEAR(0.25, view.opacity.shadow, 1e-6);
    CHECK(view.has_shadow);
    wm_desktop_destroy(desktop);
}

static void test_fullscreen_uses_requested_area(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    wm_rect area = {0, 0, 3840, 1080};
    wm_window_set_fullscreen_area(desktop, window, &area);
    wm_request_state(desktop, window, &(wm_state_change) {WM_STATE_FULLSCREEN, 0});
    CHECK_RECT_EQ(area, fake_client_of(window));
    wm_window_set_fullscreen_area(desktop, window, NULL);
    CHECK_RECT_EQ(((wm_rect) {0, 0, 1920, 1080}), fake_client_of(window));
    wm_rect gone = {5000, 0, 1920, 1080};
    wm_window_set_fullscreen_area(desktop, window, &gone);
    CHECK_RECT_EQ(((wm_rect) {0, 0, 1920, 1080}), fake_client_of(window));
    wm_desktop_destroy(desktop);
}

static void test_locked_opacity_ignores_settings(void)
{
    fake_world world;
    wm_config config = fake_config();
    config.compositing.inactive_opacity_percent = 50;
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window_desc desc = fake_desc(1, small);
    desc.opacity = 0.8;
    desc.opacity_locked = true;
    wm_window *first = wm_window_map(desktop, &desc);
    fake_map(desktop, 2, small);
    CHECK_DOUBLE_NEAR(0.8, view_of(first).opacity.content, 1e-3);
    wm_desktop_destroy(desktop);
}

static void test_opaque_undecorated_window_has_shadow(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window_desc desc = fake_desc(1, small);
    desc.capabilities &= ~(uint32_t) WM_CAP_DECORATED;
    wm_window *window = wm_window_map(desktop, &desc);
    CHECK(!view_of(window).has_shadow);
    wm_window_set_opaque(desktop, window, true);
    CHECK(view_of(window).has_shadow);
    wm_desktop_destroy(desktop);
}

static void test_compositing_disabled_is_opaque(void)
{
    fake_world world;
    wm_config config = fake_config();
    config.compositing.enabled = false;
    config.compositing.inactive_opacity_percent = 50;
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *first = fake_map(desktop, 1, small);
    fake_map(desktop, 2, small);
    CHECK_DOUBLE_NEAR(1.0, view_of(first).opacity.content, 1e-6);
    CHECK(!view_of(first).has_shadow);
    wm_desktop_destroy(desktop);
}

static void test_decor_serial_changes_only_with_inputs(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    unsigned serial = view_of(window).decor_serial;
    wm_window_set_opacity(desktop, window, 0.5);
    CHECK_INT_EQ(serial, view_of(window).decor_serial);
    wm_window_set_title(desktop, window, "another title");
    CHECK(view_of(window).decor_serial != serial);
    wm_desktop_destroy(desktop);
}

static const check_case CASES[] = {
    {"maximize_key_and_restore", test_maximize_key_and_restore},
    {"maximize_keeps_borders_without_borderless",
     test_maximize_keeps_borders_without_borderless},
    {"vertical_maximize", test_vertical_maximize},
    {"fullscreen_covers_output_without_frame", test_fullscreen_covers_output_without_frame},
    {"shade_keeps_only_title_and_bottom", test_shade_keeps_only_title_and_bottom},
    {"minimize_hides_transients", test_minimize_hides_transients},
    {"sticky_window_visible_everywhere", test_sticky_window_visible_everywhere},
    {"inactive_opacity_and_shadow", test_inactive_opacity_and_shadow},
    {"opaque_undecorated_window_has_shadow", test_opaque_undecorated_window_has_shadow},
    {"locked_opacity_ignores_settings", test_locked_opacity_ignores_settings},
    {"fullscreen_uses_requested_area", test_fullscreen_uses_requested_area},
    {"compositing_disabled_is_opaque", test_compositing_disabled_is_opaque},
    {"decor_serial_changes_only_with_inputs", test_decor_serial_changes_only_with_inputs},
};

int main(void)
{
    return check_run(CASES, sizeof(CASES) / sizeof(CASES[0]));
}
