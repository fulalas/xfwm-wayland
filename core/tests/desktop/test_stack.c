#include "check.h"
#include "fake_input.h"

static const wm_rect small = {0, 0, 400, 300};

static wm_window *map_typed(wm_desktop *desktop, int id, wm_window_type type)
{
    wm_window_desc desc = fake_desc(id, small);
    desc.type = type;
    return wm_window_map(desktop, &desc);
}

static void check_order(const fake_world *world, const int *ids, size_t count)
{
    CHECK_INT_EQ(count, world->stacking_count);
    for (size_t i = 0; i < count && i < world->stacking_count; i++) {
        CHECK(world->stacking[i] == fake_handle(ids[i]));
    }
}

static void test_raise_brings_transients(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *parent = fake_map(desktop, 1, small);
    fake_map_dialog(desktop, 2, parent);
    fake_map(desktop, 3, small);
    fake_click click = {WM_TARGET_CLIENT, parent, {800, 500}, WM_MOUSE_LEFT, 0};
    fake_press(&world, desktop, &click);
    static const int expected[] = {3, 1, 2};
    check_order(&world, expected, 3);
    wm_desktop_destroy(desktop);
}

static void test_layers_order_types(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    map_typed(desktop, 1, WM_TYPE_DOCK);
    wm_window *normal = map_typed(desktop, 2, WM_TYPE_NORMAL);
    map_typed(desktop, 3, WM_TYPE_DESKTOP);
    fake_click click = {WM_TARGET_CLIENT, normal, {800, 500}, WM_MOUSE_LEFT, 0};
    fake_press(&world, desktop, &click);
    static const int expected[] = {3, 2, 1};
    check_order(&world, expected, 3);
    CHECK_INT_EQ(WM_LAYER_NORMAL, wm_window_current_view(normal).layer);
    wm_desktop_destroy(desktop);
}

static void test_focused_fullscreen_goes_above_docks(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    map_typed(desktop, 2, WM_TYPE_DOCK);
    wm_request_state(desktop, window, &(wm_state_change) {WM_STATE_FULLSCREEN, 0});
    CHECK_INT_EQ(WM_LAYER_FULLSCREEN, wm_window_current_view(window).layer);
    static const int expected[] = {2, 1};
    check_order(&world, expected, 2);
    wm_desktop_destroy(desktop);
}

static void test_unfocused_fullscreen_drops_to_normal(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    wm_request_state(desktop, window, &(wm_state_change) {WM_STATE_FULLSCREEN, 0});
    fake_map(desktop, 2, small);
    CHECK_INT_EQ(WM_LAYER_NORMAL, wm_window_current_view(window).layer);
    static const int expected[] = {1, 2};
    check_order(&world, expected, 2);
    wm_desktop_destroy(desktop);
}

static void test_above_window_stays_on_top(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *above = fake_map(desktop, 1, small);
    wm_state_change change = {WM_STATE_ABOVE, 0};
    wm_request_state(desktop, above, &change);
    fake_map(desktop, 2, small);
    static const int expected[] = {2, 1};
    check_order(&world, expected, 2);
    CHECK_INT_EQ(WM_LAYER_ABOVE, wm_window_current_view(above).layer);
    wm_desktop_destroy(desktop);
}

static void test_middle_click_on_title_lowers(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    fake_map(desktop, 1, small);
    wm_window *second = fake_map(desktop, 2, small);
    fake_click click = fake_on_part(second, WM_PART_TITLE, WM_MOUSE_MIDDLE);
    CHECK_INT_EQ(WM_CONSUMED, fake_press(&world, desktop, &click));
    static const int expected[] = {2, 1};
    check_order(&world, expected, 2);
    wm_desktop_destroy(desktop);
}

static void test_transient_of_above_window_inherits_layer(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *parent = fake_map(desktop, 1, small);
    wm_state_change change = {WM_STATE_ABOVE, 0};
    wm_request_state(desktop, parent, &change);
    wm_window *dialog = fake_map_dialog(desktop, 2, parent);
    CHECK_INT_EQ(WM_LAYER_ABOVE, wm_window_current_view(dialog).layer);
    wm_desktop_destroy(desktop);
}

static void test_raising_top_window_keeps_stacking_unchanged(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    fake_map(desktop, 1, small);
    wm_window *top = fake_map(desktop, 2, small);
    int before = world.stacking_calls;
    fake_click click = {WM_TARGET_CLIENT, top, {100, 100}, WM_MOUSE_LEFT, 0};
    fake_press(&world, desktop, &click);
    CHECK_INT_EQ(before, world.stacking_calls);
    wm_desktop_destroy(desktop);
}

int main(void)
{
    static const check_case cases[] = {
        {"raise_brings_transients", test_raise_brings_transients},
        {"layers_order_types", test_layers_order_types},
        {"focused_fullscreen_goes_above_docks", test_focused_fullscreen_goes_above_docks},
        {"unfocused_fullscreen_drops_to_normal", test_unfocused_fullscreen_drops_to_normal},
        {"above_window_stays_on_top", test_above_window_stays_on_top},
        {"middle_click_on_title_lowers", test_middle_click_on_title_lowers},
        {"transient_of_above_window_inherits_layer",
         test_transient_of_above_window_inherits_layer},
        {"raising_top_window_keeps_stacking_unchanged",
         test_raising_top_window_keeps_stacking_unchanged},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
