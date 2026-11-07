#include "check.h"
#include "cursor/xcursor_resize.h"

static void test_length_follows_target_size(void)
{
    CHECK_INT_EQ(38, xcursor_resized_length(32, 38, 32));
    CHECK_INT_EQ(19, xcursor_resized_length(16, 38, 32));
    CHECK_INT_EQ(1, xcursor_resized_length(1, 2, 64));
}

static void test_resize_copies_nearest_pixels(void)
{
    const uint32_t source_data[] = {1, 2, 3, 4};
    xcursor_pixels source = {source_data, {2, 2}};
    uint32_t target[9];
    xcursor_resize(&source, target, (wm_size) {3, 3});
    const uint32_t expected[] = {1, 1, 2, 1, 1, 2, 3, 3, 4};
    for (int i = 0; i < 9; i++) {
        CHECK_INT_EQ(expected[i], target[i]);
    }
}

static void test_resize_never_blends_colors(void)
{
    const uint32_t source_data[] = {0xff000000, 0xffffffff};
    xcursor_pixels source = {source_data, {2, 1}};
    uint32_t target[5];
    xcursor_resize(&source, target, (wm_size) {5, 1});
    for (int i = 0; i < 5; i++) {
        CHECK(target[i] == 0xff000000 || target[i] == 0xffffffff);
    }
}

int main(void)
{
    const check_case cases[] = {
        {"length_follows_target_size", test_length_follows_target_size},
        {"resize_copies_nearest_pixels", test_resize_copies_nearest_pixels},
        {"resize_never_blends_colors", test_resize_never_blends_colors},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
