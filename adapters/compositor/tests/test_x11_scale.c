#include "check.h"
#include "xwayland/x11_scale.h"

static void test_scale_one_keeps_values(void)
{
    wm_rect rect = {-7, 13, 1001, 1};
    CHECK_RECT_EQ(rect, x11_scale_rect_to_x(1.0, rect));
    CHECK_RECT_EQ(rect, x11_scale_rect_from_x(1.0, rect));
}

static void test_fractional_scale_round_trips_logical_values(void)
{
    for (int value = -3000; value <= 3000; value++) {
        CHECK_INT_EQ(value, x11_scale_from_x(1.5, x11_scale_to_x(1.5, value)));
        CHECK_INT_EQ(value, x11_scale_from_x(1.25, x11_scale_to_x(1.25, value)));
        CHECK_INT_EQ(value, x11_scale_from_x(1.75, x11_scale_to_x(1.75, value)));
    }
}

static void test_screen_at_one_and_a_half(void)
{
    wm_rect screen = {0, 0, 2560, 1440};
    CHECK_RECT_EQ(((wm_rect){0, 0, 3840, 2160}), x11_scale_rect_to_x(1.5, screen));
    wm_rect panel_strut = {0, 2112, 3840, 48};
    CHECK_RECT_EQ(((wm_rect){0, 1408, 2560, 32}), x11_scale_rect_from_x(1.5, panel_strut));
}

static void test_tiny_sizes_stay_visible(void)
{
    wm_size size = x11_scale_size_from_x(3.0, (wm_size){1, 0});
    CHECK_INT_EQ(1, size.width);
    CHECK_INT_EQ(0, size.height);
}

static void test_size_limits_stay_inside_x_limits(void)
{
    wm_size max = x11_scale_size_from_x_down(1.5, (wm_size){301, 300});
    wm_size min = x11_scale_size_from_x_up(1.5, (wm_size){301, 300});
    CHECK(x11_scale_to_x(1.5, max.width) <= 301);
    CHECK_INT_EQ(200, max.height);
    CHECK(x11_scale_to_x(1.5, min.width) >= 301);
    CHECK_INT_EQ(200, min.height);
    CHECK_INT_EQ(0, x11_scale_size_from_x_down(1.5, (wm_size){0, 0}).width);
}

static void test_app_coordinates_kept_when_they_round_the_same(void)
{
    wm_rect hint = {295, 136, 413, 175};
    wm_rect logical = x11_scale_rect_from_x(1.5, hint);
    CHECK_RECT_EQ(hint, x11_scale_rect_to_x_near(1.5, logical, hint));
    logical.x += 1;
    CHECK_INT_EQ(x11_scale_to_x(1.5, logical.x), x11_scale_rect_to_x_near(1.5, logical, hint).x);
}

static void test_touching_windows_stay_touching(void)
{
    for (int left = 0; left < 300; left++) {
        for (int width = 2; width < 40; width++) {
            wm_rect a = {left, 0, width, 1};
            wm_size size = x11_scale_placed_size_from_x(1.5, a);
            CHECK_INT_EQ(x11_scale_from_x(1.5, left + width),
                         x11_scale_from_x(1.5, left) + size.width);
        }
    }
}

static const check_case CASES[] = {
    {"touching_windows_stay_touching", test_touching_windows_stay_touching},
    {"app_coordinates_kept_when_they_round_the_same",
     test_app_coordinates_kept_when_they_round_the_same},
    {"size_limits_stay_inside_x_limits", test_size_limits_stay_inside_x_limits},
    {"scale_one_keeps_values", test_scale_one_keeps_values},
    {"fractional_scale_round_trips_logical_values",
     test_fractional_scale_round_trips_logical_values},
    {"screen_at_one_and_a_half", test_screen_at_one_and_a_half},
    {"tiny_sizes_stay_visible", test_tiny_sizes_stay_visible},
};

int main(void)
{
    return check_run(CASES, sizeof(CASES) / sizeof(CASES[0]));
}
