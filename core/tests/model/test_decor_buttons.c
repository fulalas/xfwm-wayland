#include "check.h"
#include "decor_fixture.h"

static wm_rect button_rect(const decor_fixture *fixture, wm_button button)
{
    wm_decor_layout layout = fixture_layout(fixture);
    wm_part button_part = (wm_part) (WM_PART_BUTTON_FIRST + button);
    const wm_part_rect *part = fixture_find_part(&layout, button_part);
    return part == NULL ? (wm_rect) {0, 0, 0, 0} : part->rect;
}

static bool button_shown(const decor_fixture *fixture, wm_button button)
{
    wm_decor_layout layout = fixture_layout(fixture);
    return fixture_find_part(&layout, (wm_part) (WM_PART_BUTTON_FIRST + button)) != NULL;
}

static wm_rect button_at_x(int x)
{
    return (wm_rect) {x, 2, FIXTURE_BUTTON, FIXTURE_BUTTON};
}

static void test_default_layout_puts_menu_left_and_others_right(void)
{
    decor_fixture fixture = fixture_create();
    CHECK_RECT_EQ(button_at_x(4), button_rect(&fixture, WM_BUTTON_MENU));
    CHECK_RECT_EQ(button_at_x(140), button_rect(&fixture, WM_BUTTON_SHADE));
    CHECK_RECT_EQ(button_at_x(156), button_rect(&fixture, WM_BUTTON_HIDE));
    CHECK_RECT_EQ(button_at_x(172), button_rect(&fixture, WM_BUTTON_MAXIMIZE));
    CHECK_RECT_EQ(button_at_x(188), button_rect(&fixture, WM_BUTTON_CLOSE));
}

static void test_button_missing_from_layout_is_hidden(void)
{
    decor_fixture fixture = fixture_create();
    CHECK(!button_shown(&fixture, WM_BUTTON_STICK));
}

static void test_spacing_and_offset_move_buttons_inward(void)
{
    decor_fixture fixture = fixture_create();
    fixture.config.button_spacing = 2;
    fixture.config.button_offset = 3;
    CHECK_RECT_EQ(button_at_x(7), button_rect(&fixture, WM_BUTTON_MENU));
    CHECK_RECT_EQ(button_at_x(185), button_rect(&fixture, WM_BUTTON_CLOSE));
    CHECK_RECT_EQ(button_at_x(167), button_rect(&fixture, WM_BUTTON_MAXIMIZE));
    CHECK_RECT_EQ(button_at_x(149), button_rect(&fixture, WM_BUTTON_HIDE));
    CHECK_RECT_EQ(button_at_x(131), button_rect(&fixture, WM_BUTTON_SHADE));
}

static void test_narrow_window_drops_buttons_that_do_not_fit(void)
{
    decor_fixture fixture = fixture_create();
    fixture.window.client_size.width = 40;
    CHECK_RECT_EQ(button_at_x(4), button_rect(&fixture, WM_BUTTON_MENU));
    CHECK_RECT_EQ(button_at_x(28), button_rect(&fixture, WM_BUTTON_CLOSE));
    CHECK(!button_shown(&fixture, WM_BUTTON_MAXIMIZE));
    CHECK(!button_shown(&fixture, WM_BUTTON_HIDE));
    CHECK(!button_shown(&fixture, WM_BUTTON_SHADE));
}

static void test_unavailable_button_leaves_no_gap(void)
{
    decor_fixture fixture = fixture_create();
    fixture.window.can_maximize = false;
    CHECK(!button_shown(&fixture, WM_BUTTON_MAXIMIZE));
    CHECK_RECT_EQ(button_at_x(172), button_rect(&fixture, WM_BUTTON_HIDE));
    CHECK_RECT_EQ(button_at_x(156), button_rect(&fixture, WM_BUTTON_SHADE));
}

static void test_stick_button_needs_menu(void)
{
    decor_fixture fixture = fixture_create();
    fixture_set_layout(&fixture, "T|C");
    CHECK(button_shown(&fixture, WM_BUTTON_STICK));
    fixture.window.has_menu = false;
    CHECK(!button_shown(&fixture, WM_BUTTON_STICK));
}

static void test_layout_without_separator_puts_all_left(void)
{
    decor_fixture fixture = fixture_create();
    fixture_set_layout(&fixture, "CM");
    CHECK_RECT_EQ(button_at_x(4), button_rect(&fixture, WM_BUTTON_CLOSE));
    CHECK_RECT_EQ(button_at_x(20), button_rect(&fixture, WM_BUTTON_MAXIMIZE));
}

static void test_unknown_letters_are_ignored(void)
{
    decor_fixture fixture = fixture_create();
    fixture_set_layout(&fixture, "X|?C");
    CHECK_RECT_EQ(button_at_x(188), button_rect(&fixture, WM_BUTTON_CLOSE));
    CHECK(!button_shown(&fixture, WM_BUTTON_MENU));
}

static void test_maximized_borderless_uses_maximized_offset(void)
{
    decor_fixture fixture = fixture_create();
    fixture_maximize(&fixture);
    fixture.config.button_offset = 9;
    fixture.config.maximized_offset = 5;
    CHECK_RECT_EQ(button_at_x(5), button_rect(&fixture, WM_BUTTON_MENU));
    CHECK_RECT_EQ(button_at_x(179), button_rect(&fixture, WM_BUTTON_CLOSE));
}

static void test_negative_maximized_offset_is_ignored(void)
{
    decor_fixture fixture = fixture_create();
    fixture_maximize(&fixture);
    fixture.config.maximized_offset = -5;
    CHECK_RECT_EQ(button_at_x(0), button_rect(&fixture, WM_BUTTON_MENU));
}

static void test_border_top_lifts_buttons_when_maximized(void)
{
    decor_fixture fixture = fixture_create();
    fixture_maximize(&fixture);
    fixture.config.border_top = 2;
    CHECK_INT_EQ(0, button_rect(&fixture, WM_BUTTON_CLOSE).y);
}

static void test_inactive_layout_uses_inactive_sizes(void)
{
    decor_fixture fixture = fixture_create();
    fixture.window.state = WM_FRAME_INACTIVE;
    fixture.theme.button[WM_BUTTON_CLOSE][WM_IMAGE_INACTIVE] = (wm_size) {10, 12};
    CHECK_RECT_EQ(((wm_rect) {194, 4, 10, 12}), button_rect(&fixture, WM_BUTTON_CLOSE));
}

int main(void)
{
    static const check_case cases[] = {
        {"default_layout_puts_menu_left_and_others_right",
         test_default_layout_puts_menu_left_and_others_right},
        {"button_missing_from_layout_is_hidden", test_button_missing_from_layout_is_hidden},
        {"spacing_and_offset_move_buttons_inward", test_spacing_and_offset_move_buttons_inward},
        {"narrow_window_drops_buttons_that_do_not_fit",
         test_narrow_window_drops_buttons_that_do_not_fit},
        {"unavailable_button_leaves_no_gap", test_unavailable_button_leaves_no_gap},
        {"stick_button_needs_menu", test_stick_button_needs_menu},
        {"layout_without_separator_puts_all_left", test_layout_without_separator_puts_all_left},
        {"unknown_letters_are_ignored", test_unknown_letters_are_ignored},
        {"maximized_borderless_uses_maximized_offset",
         test_maximized_borderless_uses_maximized_offset},
        {"negative_maximized_offset_is_ignored", test_negative_maximized_offset_is_ignored},
        {"border_top_lifts_buttons_when_maximized", test_border_top_lifts_buttons_when_maximized},
        {"inactive_layout_uses_inactive_sizes", test_inactive_layout_uses_inactive_sizes},
    };
    return check_run(cases, sizeof cases / sizeof cases[0]);
}
