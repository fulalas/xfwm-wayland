#include "check.h"
#include "output/scale_guess.h"

static double guess(wm_size pixels, wm_size millimeters)
{
    return scale_guess(pixels, millimeters);
}

static void test_laptop_panels(void)
{
    CHECK_DOUBLE_NEAR(1.5, guess((wm_size){2560, 1600}, (wm_size){302, 189}), 0.001);
    CHECK_DOUBLE_NEAR(2.5, guess((wm_size){3840, 2160}, (wm_size){294, 165}), 0.001);
}

static void test_desktop_monitors(void)
{
    CHECK_DOUBLE_NEAR(1.0, guess((wm_size){1920, 1080}, (wm_size){531, 299}), 0.001);
    CHECK_DOUBLE_NEAR(1.25, guess((wm_size){3840, 2160}, (wm_size){708, 398}), 0.001);
    CHECK_DOUBLE_NEAR(1.5, guess((wm_size){3840, 2160}, (wm_size){597, 336}), 0.001);
}

static void test_unknown_or_aspect_only_size(void)
{
    CHECK_DOUBLE_NEAR(1.0, guess((wm_size){3840, 2160}, (wm_size){0, 0}), 0.001);
    CHECK_DOUBLE_NEAR(1.0, guess((wm_size){3840, 2160}, (wm_size){16, 9}), 0.001);
    CHECK_DOUBLE_NEAR(1.0, guess((wm_size){3840, 2160}, (wm_size){1600, 900}), 0.001);
}

static void test_televisions(void)
{
    CHECK_DOUBLE_NEAR(1.0, guess((wm_size){3840, 2160}, (wm_size){1594, 897}), 0.001);
}

static void test_implausible_density(void)
{
    CHECK_DOUBLE_NEAR(1.0, guess((wm_size){3840, 2160}, (wm_size){50, 30}), 0.001);
}

static void test_capped(void)
{
    CHECK_DOUBLE_NEAR(3.0, guess((wm_size){3840, 2160}, (wm_size){221, 125}), 0.001);
}

int main(void)
{
    const check_case cases[] = {
        {"laptop_panels", test_laptop_panels},
        {"desktop_monitors", test_desktop_monitors},
        {"unknown_or_aspect_only_size", test_unknown_or_aspect_only_size},
        {"televisions", test_televisions},
        {"implausible_density", test_implausible_density},
        {"capped", test_capped},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
