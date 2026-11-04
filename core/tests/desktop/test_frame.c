#include "check.h"
#include "fake_input.h"

static const wm_rect small = {0, 0, 400, 300};

static void click(fake_world *world, wm_desktop *desktop, const fake_click *where)
{
    fake_press(world, desktop, where);
    fake_release(world, desktop, where);
}

static wm_part button_part(wm_button button)
{
    return (wm_part) (WM_PART_BUTTON_FIRST + button);
}

static wm_button_image close_image(const wm_window *window)
{
    wm_window_view view = wm_window_current_view(window);
    for (size_t i = 0; i < view.decor->op_count; i++) {
        const wm_pixmap_ref *ref = &view.decor->ops[i].pixmap;
        if (ref->kind == WM_PIXMAP_BUTTON && ref->index == WM_BUTTON_CLOSE) {
            return (wm_button_image) ref->image;
        }
    }
    return WM_IMAGE_COUNT;
}

static void test_double_click_on_title_maximizes(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    fake_click title = fake_on_part(window, WM_PART_TITLE, WM_MOUSE_LEFT);
    click(&world, desktop, &title);
    fake_advance(&world, desktop, 50);
    click(&world, desktop, &title);
    CHECK(wm_window_current_view(window).maximized);
    wm_desktop_destroy(desktop);
}

static void test_slow_clicks_do_not_maximize(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    fake_click title = fake_on_part(window, WM_PART_TITLE, WM_MOUSE_LEFT);
    click(&world, desktop, &title);
    fake_advance(&world, desktop, 300);
    click(&world, desktop, &title);
    fake_advance(&world, desktop, 300);
    CHECK(!wm_window_current_view(window).maximized);
    wm_desktop_destroy(desktop);
}

static void test_close_button_acts_on_release_over_it(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    fake_click button = fake_on_part(window, button_part(WM_BUTTON_CLOSE), WM_MOUSE_LEFT);
    CHECK_INT_EQ(WM_CONSUMED, fake_press(&world, desktop, &button));
    CHECK(world.closed == NULL);
    fake_release(&world, desktop, &button);
    CHECK(world.closed == fake_handle(1));
    wm_desktop_destroy(desktop);
}

static void test_close_button_release_elsewhere_cancels(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    fake_click button = fake_on_part(window, button_part(WM_BUTTON_CLOSE), WM_MOUSE_LEFT);
    fake_press(&world, desktop, &button);
    fake_click away = fake_on_part(window, WM_PART_TITLE, WM_MOUSE_LEFT);
    fake_motion(&world, desktop, &away);
    fake_release(&world, desktop, &away);
    CHECK(world.closed == NULL);
    wm_desktop_destroy(desktop);
}

static void test_middle_click_maximize_button_is_vertical(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    fake_click button = fake_on_part(window, button_part(WM_BUTTON_MAXIMIZE), WM_MOUSE_MIDDLE);
    click(&world, desktop, &button);
    wm_window_view view = wm_window_current_view(window);
    CHECK_INT_EQ(1056, view.client.height);
    CHECK_INT_EQ(400, view.client.width);
    wm_desktop_destroy(desktop);
}

static void test_hover_uses_prelight_image(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_theme_metrics theme = fake_theme();
    theme.button[WM_BUTTON_CLOSE][WM_IMAGE_PRELIGHT] = (wm_size) {FAKE_BUTTON, FAKE_BUTTON};
    wm_desktop_set_appearance(desktop, &theme, &config);
    wm_window *window = fake_map(desktop, 1, small);
    fake_click over = fake_on_part(window, button_part(WM_BUTTON_CLOSE), 0);
    fake_motion(&world, desktop, &over);
    CHECK_INT_EQ(WM_IMAGE_PRELIGHT, close_image(window));
    fake_click title = fake_on_part(window, WM_PART_TITLE, 0);
    fake_motion(&world, desktop, &title);
    CHECK_INT_EQ(WM_IMAGE_ACTIVE, close_image(window));
    wm_desktop_destroy(desktop);
}

static void test_menu_button_opens_menu_after_click(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    fake_click button = fake_on_part(window, button_part(WM_BUTTON_MENU), WM_MOUSE_LEFT);
    click(&world, desktop, &button);
    CHECK(!world.menu_visible);
    fake_advance(&world, desktop, 300);
    CHECK(world.menu_visible);
    CHECK_INT_EQ(398, world.menu.main.anchor.y);
    wm_desktop_destroy(desktop);
}

static void test_double_click_menu_button_closes(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    fake_click button = fake_on_part(window, button_part(WM_BUTTON_MENU), WM_MOUSE_LEFT);
    click(&world, desktop, &button);
    click(&world, desktop, &button);
    CHECK(world.closed == fake_handle(1));
    CHECK(!world.menu_visible);
    wm_desktop_destroy(desktop);
}

static void test_right_click_on_title_opens_menu(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    fake_click title = fake_on_part(window, WM_PART_TITLE, WM_MOUSE_RIGHT);
    click(&world, desktop, &title);
    CHECK(world.menu_visible);
    CHECK_INT_EQ(title.point.x, world.menu.main.anchor.x);
    wm_desktop_destroy(desktop);
}

static void test_title_scroll_shades(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    fake_click title = fake_on_part(window, WM_PART_TITLE, 0);
    fake_scroll(&world, desktop, &title, -1);
    CHECK(wm_window_current_view(window).shaded);
    fake_scroll(&world, desktop, &title, 1);
    CHECK(!wm_window_current_view(window).shaded);
    wm_desktop_destroy(desktop);
}

static void test_easy_click_scroll_on_title_changes_opacity(void)
{
    fake_world world;
    wm_config config = fake_config();
    config.mouse.zoom_desktop = false;
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    fake_click title = fake_on_part(window, WM_PART_TITLE, 0);
    title.modifiers = WM_MOD_ALT;
    fake_scroll(&world, desktop, &title, 1);
    CHECK_DOUBLE_NEAR(1.0 - 0x16000000 / 4294967295.0,
                      wm_window_current_view(window).opacity.content, 1e-6);
    wm_desktop_destroy(desktop);
}

static void test_corner_hover_sets_resize_cursor(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    fake_click corner = fake_on_part(window, WM_PART_CORNER_BOTTOM_RIGHT, 0);
    CHECK_INT_EQ(WM_CONSUMED, fake_motion(&world, desktop, &corner));
    CHECK_INT_EQ(WM_CURSOR_RESIZE_BOTTOM_RIGHT, world.cursor);
    wm_desktop_destroy(desktop);
}

static void test_double_click_on_side_fills(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    fake_click side = fake_on_part(window, WM_PART_SIDE_LEFT, WM_MOUSE_LEFT);
    click(&world, desktop, &side);
    click(&world, desktop, &side);
    CHECK_RECT_EQ(((wm_rect) {4, 398, 1912, 300}), wm_window_current_view(window).client);
    wm_desktop_destroy(desktop);
}

static void test_easy_click_right_resizes_nearest_edge(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    fake_click where = {WM_TARGET_CLIENT, window, {1150, 540}, WM_MOUSE_RIGHT, WM_MOD_ALT};
    fake_press(&world, desktop, &where);
    where.point.x += 40;
    fake_motion(&world, desktop, &where);
    fake_release(&world, desktop, &where);
    CHECK_RECT_EQ(((wm_rect) {760, 398, 440, 300}), wm_window_current_view(window).client);
    wm_desktop_destroy(desktop);
}

int main(void)
{
    static const check_case cases[] = {
        {"double_click_on_title_maximizes", test_double_click_on_title_maximizes},
        {"slow_clicks_do_not_maximize", test_slow_clicks_do_not_maximize},
        {"close_button_acts_on_release_over_it", test_close_button_acts_on_release_over_it},
        {"close_button_release_elsewhere_cancels",
         test_close_button_release_elsewhere_cancels},
        {"middle_click_maximize_button_is_vertical",
         test_middle_click_maximize_button_is_vertical},
        {"hover_uses_prelight_image", test_hover_uses_prelight_image},
        {"menu_button_opens_menu_after_click", test_menu_button_opens_menu_after_click},
        {"double_click_menu_button_closes", test_double_click_menu_button_closes},
        {"right_click_on_title_opens_menu", test_right_click_on_title_opens_menu},
        {"title_scroll_shades", test_title_scroll_shades},
        {"easy_click_scroll_on_title_changes_opacity",
         test_easy_click_scroll_on_title_changes_opacity},
        {"corner_hover_sets_resize_cursor", test_corner_hover_sets_resize_cursor},
        {"double_click_on_side_fills", test_double_click_on_side_fills},
        {"easy_click_right_resizes_nearest_edge", test_easy_click_right_resizes_nearest_edge},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
