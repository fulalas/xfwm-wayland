#include "check.h"
#include "fake_ports.h"

static const wm_rect small = {0, 0, 400, 300};

static wm_desktop *start(fake_world *world)
{
    fake_world_init(world);
    wm_config config = fake_config();
    return fake_desktop_create(world, &config);
}

static wm_window *map_with_capabilities(wm_desktop *desktop, uint32_t capabilities)
{
    wm_window_desc desc = fake_desc(1, small);
    desc.capabilities = capabilities;
    return wm_window_map(desktop, &desc);
}

static void test_normal_window_allows_everything(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    wm_window *window = fake_map(desktop, 1, small);
    uint32_t all = WM_ALLOW_MOVE | WM_ALLOW_RESIZE | WM_ALLOW_MINIMIZE | WM_ALLOW_SHADE
                   | WM_ALLOW_STICK | WM_ALLOW_MAXIMIZE | WM_ALLOW_FULLSCREEN
                   | WM_ALLOW_CHANGE_WORKSPACE | WM_ALLOW_CLOSE | WM_ALLOW_ABOVE
                   | WM_ALLOW_BELOW;
    CHECK_INT_EQ(all, wm_window_current_view(window).allowed_actions);
    wm_desktop_destroy(desktop);
}

static void test_missing_capabilities_remove_actions(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    uint32_t capabilities = WM_CAP_ALL & ~(uint32_t) (WM_CAP_MAXIMIZE | WM_CAP_MINIMIZE);
    wm_window *window = map_with_capabilities(desktop, capabilities);
    uint32_t actions = wm_window_current_view(window).allowed_actions;
    CHECK(!(actions & WM_ALLOW_MAXIMIZE));
    CHECK(!(actions & WM_ALLOW_MINIMIZE));
    CHECK(actions & WM_ALLOW_MOVE);
    wm_desktop_destroy(desktop);
}

static void test_maximized_window_cannot_resize(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    wm_window *window = fake_map(desktop, 1, small);
    wm_request_state(desktop, window, &(wm_state_change) {WM_STATE_MAXIMIZED, 0});
    CHECK(!(wm_window_current_view(window).allowed_actions & WM_ALLOW_RESIZE));
    wm_desktop_destroy(desktop);
}

static void test_undecorated_window_cannot_shade(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    wm_window *window = map_with_capabilities(desktop, WM_CAP_ALL & ~(uint32_t) WM_CAP_DECORATED);
    CHECK(!(wm_window_current_view(window).allowed_actions & WM_ALLOW_SHADE));
    wm_desktop_destroy(desktop);
}

static void test_estimated_extents_match_mapped_window(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    wm_window *window = fake_map(desktop, 1, small);
    wm_extents mapped = wm_window_current_view(window).decor->extents;
    wm_extents estimate = wm_desktop_frame_extents(desktop, WM_CAP_ALL);
    CHECK_INT_EQ(mapped.left, estimate.left);
    CHECK_INT_EQ(mapped.right, estimate.right);
    CHECK_INT_EQ(mapped.top, estimate.top);
    CHECK_INT_EQ(mapped.bottom, estimate.bottom);
    CHECK(estimate.top > 0);
    wm_desktop_destroy(desktop);
}

static void test_undecorated_estimate_is_empty(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    uint32_t undecorated = WM_CAP_ALL & ~(uint32_t) WM_CAP_DECORATED;
    wm_extents estimate = wm_desktop_frame_extents(desktop, undecorated);
    CHECK_INT_EQ(0, estimate.left + estimate.right + estimate.top + estimate.bottom);
    wm_desktop_destroy(desktop);
}

int main(void)
{
    static const check_case cases[] = {
        {"normal_window_allows_everything", test_normal_window_allows_everything},
        {"missing_capabilities_remove_actions", test_missing_capabilities_remove_actions},
        {"maximized_window_cannot_resize", test_maximized_window_cannot_resize},
        {"undecorated_window_cannot_shade", test_undecorated_window_cannot_shade},
        {"estimated_extents_match_mapped_window", test_estimated_extents_match_mapped_window},
        {"undecorated_estimate_is_empty", test_undecorated_estimate_is_empty},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
