#include <math.h>

#include "check.h"
#include "cursor/scalable_layout.h"

static scalable_layout layout(int cursor_size, double output_scale)
{
    scalable_request request = {24.0, 24.0, cursor_size, output_scale};
    return scalable_layout_for(&request);
}

static void test_canvas_matches_size_at_scale_one(void)
{
    CHECK_INT_EQ(24, layout(24, 1.0).pixels);
}

static void test_fractional_scale_renders_exact_pixels(void)
{
    scalable_layout result = layout(33, 1.328125);
    CHECK_INT_EQ(44, result.pixels);
    CHECK_DOUBLE_NEAR(33.0 / 24.0, result.hotspot_factor, 0.0001);
}

static void test_tiny_size_keeps_one_pixel(void)
{
    CHECK_INT_EQ(1, layout(1, 0.3).pixels);
}

static int drawn_pixels(const scalable_layout *result, float output_scale)
{
    int logical = (int) ((float) result->pixels / result->buffer_scale);
    return (int) roundf((float) logical * output_scale);
}

static void test_drawn_one_to_one_at_every_size(void)
{
    const float scales[] = {1.0f, 1.25f, 1.328125f, 1.5f, 1.75f, 2.0f};
    for (size_t i = 0; i < sizeof(scales) / sizeof(scales[0]); i++) {
        for (int size = 8; size <= 128; size++) {
            scalable_layout result = layout(size, scales[i]);
            CHECK_INT_EQ(result.pixels, drawn_pixels(&result, scales[i]));
        }
    }
}

int main(void)
{
    const check_case cases[] = {
        {"canvas_matches_size_at_scale_one", test_canvas_matches_size_at_scale_one},
        {"fractional_scale_renders_exact_pixels", test_fractional_scale_renders_exact_pixels},
        {"tiny_size_keeps_one_pixel", test_tiny_size_keeps_one_pixel},
        {"drawn_one_to_one_at_every_size", test_drawn_one_to_one_at_every_size},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
