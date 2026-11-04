#include "check.h"
#include "decor_fixture.h"

static wm_part part_at(const decor_fixture *fixture, int x, int y)
{
    wm_decor_layout layout = fixture_layout(fixture);
    return wm_decor_part_at(&layout, (wm_point) {x, y});
}

static void test_buttons_win_over_title_and_top_strip(void)
{
    decor_fixture fixture = fixture_create();
    CHECK_INT_EQ(WM_PART_BUTTON_FIRST + WM_BUTTON_CLOSE, part_at(&fixture, 190, 2));
    CHECK_INT_EQ(WM_PART_BUTTON_FIRST + WM_BUTTON_MENU, part_at(&fixture, 10, 10));
}

static void test_top_strip_wins_over_title(void)
{
    decor_fixture fixture = fixture_create();
    CHECK_INT_EQ(WM_PART_SIDE_TOP, part_at(&fixture, 100, 2));
    CHECK_INT_EQ(WM_PART_TITLE, part_at(&fixture, 100, 3));
}

static void test_corners_cover_frame_corners(void)
{
    decor_fixture fixture = fixture_create();
    CHECK_INT_EQ(WM_PART_CORNER_TOP_LEFT, part_at(&fixture, 1, 1));
    CHECK_INT_EQ(WM_PART_CORNER_TOP_RIGHT, part_at(&fixture, 207, 19));
    CHECK_INT_EQ(WM_PART_CORNER_BOTTOM_LEFT, part_at(&fixture, 0, 123));
    CHECK_INT_EQ(WM_PART_CORNER_BOTTOM_RIGHT, part_at(&fixture, 202, 118));
}

static void test_sides_surround_the_client(void)
{
    decor_fixture fixture = fixture_create();
    CHECK_INT_EQ(WM_PART_SIDE_LEFT, part_at(&fixture, 3, 60));
    CHECK_INT_EQ(WM_PART_SIDE_RIGHT, part_at(&fixture, 204, 60));
    CHECK_INT_EQ(WM_PART_SIDE_BOTTOM, part_at(&fixture, 100, 120));
}

static void test_client_area_is_no_part(void)
{
    decor_fixture fixture = fixture_create();
    CHECK_INT_EQ(WM_PART_NONE, part_at(&fixture, 100, 60));
    CHECK_INT_EQ(WM_PART_NONE, part_at(&fixture, 208, 60));
}

static void test_parts_are_listed_by_priority(void)
{
    decor_fixture fixture = fixture_create();
    wm_decor_layout layout = fixture_layout(&fixture);
    static const wm_part expected[] = {
        WM_PART_BUTTON_FIRST + WM_BUTTON_MENU,  WM_PART_BUTTON_FIRST + WM_BUTTON_SHADE,
        WM_PART_BUTTON_FIRST + WM_BUTTON_HIDE,  WM_PART_BUTTON_FIRST + WM_BUTTON_MAXIMIZE,
        WM_PART_BUTTON_FIRST + WM_BUTTON_CLOSE, WM_PART_CORNER_TOP_LEFT,
        WM_PART_CORNER_TOP_RIGHT,               WM_PART_CORNER_BOTTOM_LEFT,
        WM_PART_CORNER_BOTTOM_RIGHT,            WM_PART_SIDE_TOP,
        WM_PART_TITLE,                          WM_PART_SIDE_LEFT,
        WM_PART_SIDE_RIGHT,                     WM_PART_SIDE_BOTTOM,
    };
    CHECK_INT_EQ(sizeof expected / sizeof expected[0], layout.part_count);
    for (size_t i = 0; i < layout.part_count && i < sizeof expected / sizeof expected[0]; i++) {
        CHECK_INT_EQ(expected[i], layout.parts[i].part);
    }
}

static void test_maximized_borderless_has_only_title_and_buttons(void)
{
    decor_fixture fixture = fixture_create();
    fixture_maximize(&fixture);
    wm_decor_layout layout = fixture_layout(&fixture);
    CHECK_INT_EQ(6, layout.part_count);
    CHECK_INT_EQ(WM_PART_TITLE, part_at(&fixture, 1, 1));
    CHECK_INT_EQ(WM_PART_NONE, part_at(&fixture, 1, 60));
}

static void test_shaded_frame_has_no_left_or_right_side(void)
{
    decor_fixture fixture = fixture_create();
    fixture.window.shaded = true;
    wm_decor_layout layout = fixture_layout(&fixture);
    CHECK(fixture_find_part(&layout, WM_PART_SIDE_LEFT) == NULL);
    CHECK(fixture_find_part(&layout, WM_PART_SIDE_RIGHT) == NULL);
    CHECK_INT_EQ(WM_PART_CORNER_BOTTOM_LEFT, part_at(&fixture, 1, 21));
    CHECK_INT_EQ(WM_PART_SIDE_BOTTOM, part_at(&fixture, 100, 21));
}

static void test_frameless_window_has_no_parts(void)
{
    decor_fixture fixture = fixture_create();
    fixture.window.fullscreen = true;
    CHECK_INT_EQ(WM_PART_NONE, part_at(&fixture, 0, 0));
}

int main(void)
{
    static const check_case cases[] = {
        {"buttons_win_over_title_and_top_strip", test_buttons_win_over_title_and_top_strip},
        {"top_strip_wins_over_title", test_top_strip_wins_over_title},
        {"corners_cover_frame_corners", test_corners_cover_frame_corners},
        {"sides_surround_the_client", test_sides_surround_the_client},
        {"client_area_is_no_part", test_client_area_is_no_part},
        {"parts_are_listed_by_priority", test_parts_are_listed_by_priority},
        {"maximized_borderless_has_only_title_and_buttons",
         test_maximized_borderless_has_only_title_and_buttons},
        {"shaded_frame_has_no_left_or_right_side", test_shaded_frame_has_no_left_or_right_side},
        {"frameless_window_has_no_parts", test_frameless_window_has_no_parts},
    };
    return check_run(cases, sizeof cases / sizeof cases[0]);
}
