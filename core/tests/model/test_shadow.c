#include "check.h"
#include "wm/shadow.h"

static void test_kernel_size_is_even_and_three_radii(void)
{
    CHECK_INT_EQ(36, wm_shadow_kernel_size());
}

static void test_default_shadow_is_shifted_down(void)
{
    wm_compositing_config config = wm_config_defaults().compositing;
    wm_rect frame = {100, 50, 200, 150};
    CHECK_RECT_EQ(((wm_rect) {82, 35, 236, 183}), wm_shadow_rect(&frame, &config));
}

static void test_zero_deltas_center_the_shadow(void)
{
    wm_compositing_config config = wm_config_defaults().compositing;
    wm_rect frame = {0, 0, 10, 10};
    config.shadow_delta_y = 0;
    CHECK_RECT_EQ(((wm_rect) {-18, -18, 46, 46}), wm_shadow_rect(&frame, &config));
}

static void test_deltas_move_and_resize_the_shadow(void)
{
    wm_compositing_config config = wm_config_defaults().compositing;
    wm_rect frame = {0, 0, 10, 10};
    config.shadow_delta_x = 5;
    config.shadow_delta_y = 0;
    config.shadow_delta_width = 2;
    config.shadow_delta_height = -4;
    CHECK_RECT_EQ(((wm_rect) {-23, -18, 53, 42}), wm_shadow_rect(&frame, &config));
}

static const check_case CASES[] = {
    {"kernel_size_is_even_and_three_radii", test_kernel_size_is_even_and_three_radii},
    {"default_shadow_is_shifted_down", test_default_shadow_is_shifted_down},
    {"zero_deltas_center_the_shadow", test_zero_deltas_center_the_shadow},
    {"deltas_move_and_resize_the_shadow", test_deltas_move_and_resize_the_shadow},
};

int main(void)
{
    return check_run(CASES, sizeof(CASES) / sizeof(CASES[0]));
}
