#include "check.h"
#include "fake_input.h"

static const wm_rect small = {0, 0, 400, 300};

static wm_dispatch menu_event(wm_desktop *desktop, wm_target_kind kind, int item, bool pressed)
{
    wm_pointer_event pointer = {{0, 0}, 0, 0, {kind, NULL, item}};
    wm_pointer_motion(desktop, &pointer);
    wm_button_event event = {pointer, WM_MOUSE_LEFT, pressed};
    return wm_pointer_button(desktop, &event);
}

static void test_popup_key_builds_menu(void)
{
    static const wm_menu_label expected[] = {
        WM_MENU_MAXIMIZE, WM_MENU_MINIMIZE, WM_MENU_MINIMIZE_OTHERS, WM_MENU_MOVE,
        WM_MENU_RESIZE, WM_MENU_SEPARATOR, WM_MENU_ABOVE, WM_MENU_NORMAL, WM_MENU_BELOW,
        WM_MENU_SEPARATOR, WM_MENU_SHADE, WM_MENU_FULLSCREEN, WM_MENU_SEPARATOR, WM_MENU_STICK,
        WM_MENU_WORKSPACES, WM_MENU_SEPARATOR, WM_MENU_CLOSE,
    };
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    fake_map(desktop, 1, small);
    fake_key(desktop, FAKE_KEY_SPACE, WM_MOD_ALT);
    CHECK(world.menu_visible);
    CHECK_INT_EQ(760, world.menu.main.anchor.x);
    CHECK_INT_EQ(398, world.menu.main.anchor.y);
    CHECK_INT_EQ(17, world.menu.main.count);
    for (size_t i = 0; i < world.menu.main.count && i < 17; i++) {
        CHECK_INT_EQ(expected[i], world.menu.main.items[i].label);
    }
    CHECK_INT_EQ(WM_MENU_MARK_RADIO_ON, world.menu.main.items[7].mark);
    CHECK_INT_EQ(WM_MENU_MARK_CHECK_OFF, world.menu.main.items[13].mark);
    CHECK(world.menu.main.items[14].has_submenu);
    wm_desktop_destroy(desktop);
}

static void test_keyboard_navigation_activates_above(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    fake_key(desktop, FAKE_KEY_SPACE, WM_MOD_ALT);
    for (int i = 0; i < 6; i++) {
        fake_key(desktop, WM_KEYSYM_DOWN, 0);
    }
    CHECK_INT_EQ(6, world.menu.main.selected);
    fake_key(desktop, WM_KEYSYM_RETURN, 0);
    CHECK(!world.menu_visible);
    CHECK(wm_window_current_view(window).above);
    wm_desktop_destroy(desktop);
}

static void test_workspace_submenu_moves_window(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    fake_key(desktop, FAKE_KEY_SPACE, WM_MOD_ALT);
    menu_event(desktop, WM_TARGET_MENU, 14, true);
    CHECK(world.menu.submenu_open);
    CHECK_INT_EQ(4, world.menu.submenu.count);
    CHECK(!world.menu.submenu.items[0].sensitive);
    CHECK(world.menu.submenu.items[2].text == NULL);
    menu_event(desktop, WM_TARGET_SUBMENU, 2, false);
    CHECK(!world.menu_visible);
    CHECK_INT_EQ(2, wm_window_current_view(window).workspace);
    wm_desktop_destroy(desktop);
}

static void test_monitor_submenu_moves_to_neighbour(void)
{
    fake_world world;
    fake_world_init(&world);
    wm_rect second = {1920, 0, 1920, 1080};
    fake_world_add_output(&world, second, second);
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_create(&world, &config);
    fake_set_keymap(desktop);
    wm_window *window = fake_map(desktop, 1, small);
    fake_key(desktop, FAKE_KEY_SPACE, WM_MOD_ALT);
    CHECK_INT_EQ(WM_MENU_MONITORS, world.menu.main.items[15].label);
    menu_event(desktop, WM_TARGET_MENU, 15, false);
    CHECK_INT_EQ(1, world.menu.submenu.count);
    CHECK_INT_EQ(WM_MENU_MONITOR_RIGHT, world.menu.submenu.items[0].label);
    menu_event(desktop, WM_TARGET_SUBMENU, 0, false);
    CHECK_INT_EQ(2680, wm_window_current_view(window).client.x);
    wm_desktop_destroy(desktop);
}

static void test_click_elsewhere_closes_menu(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    fake_map(desktop, 1, small);
    fake_key(desktop, FAKE_KEY_SPACE, WM_MOD_ALT);
    CHECK_INT_EQ(WM_CONSUMED, menu_event(desktop, WM_TARGET_DESKTOP, -1, true));
    CHECK(!world.menu_visible);
    wm_desktop_destroy(desktop);
}

static void test_escape_closes_submenu_then_menu(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    fake_map(desktop, 1, small);
    fake_key(desktop, FAKE_KEY_SPACE, WM_MOD_ALT);
    menu_event(desktop, WM_TARGET_MENU, 14, false);
    fake_key(desktop, WM_KEYSYM_ESCAPE, 0);
    CHECK(world.menu_visible);
    CHECK(!world.menu.submenu_open);
    fake_key(desktop, WM_KEYSYM_ESCAPE, 0);
    CHECK(!world.menu_visible);
    wm_desktop_destroy(desktop);
}

static void test_menu_close_item(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    fake_map(desktop, 1, small);
    fake_key(desktop, FAKE_KEY_SPACE, WM_MOD_ALT);
    menu_event(desktop, WM_TARGET_MENU, 16, false);
    CHECK(world.closed == fake_handle(1));
    wm_desktop_destroy(desktop);
}

static void test_transient_menu_disables_layers_and_stick(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *parent = fake_map(desktop, 1, small);
    wm_window_desc desc = fake_desc(2, small);
    desc.type = WM_TYPE_DIALOG;
    desc.parent = parent;
    wm_window *dialog = wm_window_map(desktop, &desc);
    wm_request_window_menu(desktop, dialog, (wm_point) {5, 5});
    CHECK(!world.menu.main.items[6].sensitive);
    CHECK(!world.menu.main.items[13].sensitive);
    CHECK(!world.menu.main.items[1].sensitive);
    wm_desktop_destroy(desktop);
}

int main(void)
{
    static const check_case cases[] = {
        {"popup_key_builds_menu", test_popup_key_builds_menu},
        {"keyboard_navigation_activates_above", test_keyboard_navigation_activates_above},
        {"workspace_submenu_moves_window", test_workspace_submenu_moves_window},
        {"monitor_submenu_moves_to_neighbour", test_monitor_submenu_moves_to_neighbour},
        {"click_elsewhere_closes_menu", test_click_elsewhere_closes_menu},
        {"escape_closes_submenu_then_menu", test_escape_closes_submenu_then_menu},
        {"menu_close_item", test_menu_close_item},
        {"transient_menu_disables_layers_and_stick",
         test_transient_menu_disables_layers_and_stick},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
