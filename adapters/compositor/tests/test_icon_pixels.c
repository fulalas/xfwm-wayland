#include "check.h"
#include "icon_pixels.h"

static void test_opaque_pixel_is_unchanged(void)
{
    CHECK_INT_EQ(0xff336699u, icon_pixels_premultiply(0xff336699u));
}

static void test_transparent_pixel_becomes_zero(void)
{
    CHECK_INT_EQ(0x00000000u, icon_pixels_premultiply(0x00ffffffu));
}

static void test_half_alpha_halves_channels(void)
{
    CHECK_INT_EQ(0x80800000u, icon_pixels_premultiply(0x80ff0000u));
}

static void test_best_size_prefers_exact_match(void)
{
    const wm_size sizes[] = {{16, 16}, {64, 64}, {128, 128}};
    CHECK_INT_EQ(1, icon_pixels_best_index(sizes, 3, 64));
}

static void test_best_size_prefers_larger_than_smaller(void)
{
    const wm_size sizes[] = {{32, 32}, {256, 256}};
    CHECK_INT_EQ(1, icon_pixels_best_index(sizes, 2, 48));
}

static void test_best_size_takes_closest_larger(void)
{
    const wm_size sizes[] = {{256, 256}, {96, 96}, {48, 48}};
    CHECK_INT_EQ(1, icon_pixels_best_index(sizes, 3, 64));
}

int main(void)
{
    const check_case cases[] = {
        {"opaque_pixel_is_unchanged", test_opaque_pixel_is_unchanged},
        {"transparent_pixel_becomes_zero", test_transparent_pixel_becomes_zero},
        {"half_alpha_halves_channels", test_half_alpha_halves_channels},
        {"best_size_prefers_exact_match", test_best_size_prefers_exact_match},
        {"best_size_prefers_larger_than_smaller", test_best_size_prefers_larger_than_smaller},
        {"best_size_takes_closest_larger", test_best_size_takes_closest_larger},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
