#include "check.h"
#include "fake_input.h"

static const wm_rect small = {0, 0, 400, 300};

static wm_desktop *create_with_three(fake_world *world, const wm_config *config)
{
    wm_desktop *desktop = fake_desktop_start(world, config);
    for (int id = 1; id <= 3; id++) {
        fake_map(desktop, id, small);
    }
    return desktop;
}

static void test_cycle_lists_windows_in_focus_order(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = create_with_three(&world, &config);
    fake_key(desktop, WM_KEYSYM_TAB, WM_MOD_ALT);
    CHECK(world.cycle_visible);
    CHECK_INT_EQ(3, world.cycle.count);
    CHECK(world.cycle.items[0].window == fake_handle(3));
    CHECK(world.cycle.items[1].window == fake_handle(2));
    CHECK(world.cycle.items[2].window == fake_handle(1));
    CHECK_INT_EQ(1, world.cycle.selected);
    CHECK(world.outline_visible);
    wm_desktop_destroy(desktop);
}

static void test_releasing_modifier_activates_selection(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = create_with_three(&world, &config);
    fake_key(desktop, WM_KEYSYM_TAB, WM_MOD_ALT);
    fake_key(desktop, WM_KEYSYM_TAB, WM_MOD_ALT);
    CHECK_INT_EQ(2, world.cycle.selected);
    wm_modifiers_changed(desktop, 0);
    CHECK(!world.cycle_visible);
    CHECK(!world.outline_visible);
    CHECK(world.focused == fake_handle(1));
    wm_desktop_destroy(desktop);
}

static void test_reverse_cycle_selects_last(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = create_with_three(&world, &config);
    fake_key(desktop, WM_KEYSYM_TAB, WM_MOD_ALT | WM_MOD_SHIFT);
    CHECK_INT_EQ(2, world.cycle.selected);
    fake_key(desktop, WM_KEYSYM_TAB, WM_MOD_ALT | WM_MOD_SHIFT);
    CHECK_INT_EQ(1, world.cycle.selected);
    wm_desktop_destroy(desktop);
}

static void test_cancel_keeps_focus(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = create_with_three(&world, &config);
    fake_key(desktop, WM_KEYSYM_TAB, WM_MOD_ALT);
    fake_key(desktop, WM_KEYSYM_ESCAPE, WM_MOD_ALT);
    CHECK(!world.cycle_visible);
    CHECK(world.focused == fake_handle(3));
    wm_desktop_destroy(desktop);
}

static void test_mouse_selects_and_activates(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = create_with_three(&world, &config);
    fake_key(desktop, WM_KEYSYM_TAB, WM_MOD_ALT);
    wm_pointer_event motion = {{10, 10}, 0, WM_MOD_ALT, {WM_TARGET_CYCLE, NULL, 2}};
    wm_pointer_motion(desktop, &motion);
    CHECK_INT_EQ(2, world.cycle.selected);
    wm_button_event press = {motion, WM_MOUSE_LEFT, true};
    CHECK_INT_EQ(WM_CONSUMED, wm_pointer_button(desktop, &press));
    CHECK(world.focused == fake_handle(1));
    CHECK(!world.cycle_visible);
    wm_desktop_destroy(desktop);
}

static void test_minimized_windows_come_last(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    fake_map(desktop, 1, small);
    wm_window *second = fake_map(desktop, 2, small);
    fake_map(desktop, 3, small);
    wm_request_state(desktop, second, &(wm_state_change) {WM_STATE_MINIMIZED, 0});
    fake_key(desktop, WM_KEYSYM_TAB, WM_MOD_ALT);
    CHECK_INT_EQ(3, world.cycle.count);
    CHECK(world.cycle.items[2].window == fake_handle(2));
    CHECK(world.cycle.items[2].minimized);
    wm_desktop_destroy(desktop);
}

static void test_unmapped_window_leaves_cycle(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *first = fake_map(desktop, 1, small);
    fake_map(desktop, 2, small);
    fake_key(desktop, WM_KEYSYM_TAB, WM_MOD_ALT);
    wm_window_unmap(desktop, first);
    CHECK_INT_EQ(1, world.cycle.count);
    CHECK_INT_EQ(0, world.cycle.selected);
    wm_desktop_destroy(desktop);
}

static void test_grid_geometry(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = create_with_three(&world, &config);
    fake_key(desktop, WM_KEYSYM_TAB, WM_MOD_ALT);
    CHECK(world.cycle.preview);
    CHECK_INT_EQ(288, world.cycle.item_size);
    CHECK_INT_EQ(4, world.cycle.columns);
    CHECK_INT_EQ(1, world.cycle.rows);
    CHECK_RECT_EQ(((wm_rect) {0, 0, 1920, 1080}), world.cycle.monitor);
    wm_desktop_destroy(desktop);
}

static void test_arrow_keys_move_in_grid(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = create_with_three(&world, &config);
    fake_key(desktop, WM_KEYSYM_TAB, WM_MOD_ALT);
    fake_key(desktop, WM_KEYSYM_RIGHT, WM_MOD_ALT);
    CHECK_INT_EQ(2, world.cycle.selected);
    fake_key(desktop, WM_KEYSYM_RIGHT, WM_MOD_ALT);
    CHECK_INT_EQ(0, world.cycle.selected);
    fake_key(desktop, WM_KEYSYM_LEFT, WM_MOD_ALT);
    CHECK_INT_EQ(2, world.cycle.selected);
    wm_desktop_destroy(desktop);
}

int main(void)
{
    static const check_case cases[] = {
        {"cycle_lists_windows_in_focus_order", test_cycle_lists_windows_in_focus_order},
        {"releasing_modifier_activates_selection", test_releasing_modifier_activates_selection},
        {"reverse_cycle_selects_last", test_reverse_cycle_selects_last},
        {"cancel_keeps_focus", test_cancel_keeps_focus},
        {"mouse_selects_and_activates", test_mouse_selects_and_activates},
        {"minimized_windows_come_last", test_minimized_windows_come_last},
        {"unmapped_window_leaves_cycle", test_unmapped_window_leaves_cycle},
        {"grid_geometry", test_grid_geometry},
        {"arrow_keys_move_in_grid", test_arrow_keys_move_in_grid},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
