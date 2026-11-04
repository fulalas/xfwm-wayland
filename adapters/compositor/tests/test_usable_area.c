#include "check.h"
#include "usable_area.h"

static const wm_rect left_output = {0, 0, 1920, 1080};
static const wm_rect right_output = {1920, 0, 1280, 1024};
static const wm_rect root = {0, 0, 3200, 1080};

static strut no_strut(void)
{
    strut s = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
    return s;
}

static void test_without_strut_area_is_unchanged(void)
{
    strut s = no_strut();
    strut_context context = {left_output, root};
    CHECK_RECT_EQ(left_output, usable_area_apply_strut(left_output, &context, &s));
}

static void test_bottom_panel_reserves_its_height(void)
{
    strut s = no_strut();
    s.bottom = (strut_range){30, 0, 1919};
    strut_context context = {left_output, root};
    CHECK_RECT_EQ(((wm_rect){0, 0, 1920, 1050}),
                      usable_area_apply_strut(left_output, &context, &s));
}

static void test_strut_on_other_output_is_ignored(void)
{
    strut s = no_strut();
    s.top = (strut_range){40, 0, 1919};
    strut_context context = {right_output, root};
    CHECK_RECT_EQ(right_output, usable_area_apply_strut(right_output, &context, &s));
}

static void test_right_strut_reaches_from_root_edge(void)
{
    strut s = no_strut();
    s.right = (strut_range){64, 0, 1023};
    strut_context context = {right_output, root};
    CHECK_RECT_EQ(((wm_rect){1920, 0, 1216, 1024}),
                      usable_area_apply_strut(right_output, &context, &s));
}

static void test_left_strut_shrinks_from_the_left(void)
{
    strut s = no_strut();
    s.left = (strut_range){48, 0, 1079};
    strut_context context = {left_output, root};
    CHECK_RECT_EQ(((wm_rect){48, 0, 1872, 1080}),
                      usable_area_apply_strut(left_output, &context, &s));
}

static void test_struts_accumulate(void)
{
    strut s = no_strut();
    s.top = (strut_range){24, 0, 1919};
    s.bottom = (strut_range){30, 0, 1919};
    strut_context context = {left_output, root};
    wm_rect once = usable_area_apply_strut(left_output, &context, &s);
    CHECK_RECT_EQ(((wm_rect){0, 24, 1920, 1026}), once);
    CHECK_RECT_EQ(once, usable_area_apply_strut(once, &context, &s));
}

int main(void)
{
    const check_case cases[] = {
        {"without_strut_area_is_unchanged", test_without_strut_area_is_unchanged},
        {"bottom_panel_reserves_its_height", test_bottom_panel_reserves_its_height},
        {"strut_on_other_output_is_ignored", test_strut_on_other_output_is_ignored},
        {"right_strut_reaches_from_root_edge", test_right_strut_reaches_from_root_edge},
        {"left_strut_shrinks_from_the_left", test_left_strut_shrinks_from_the_left},
        {"struts_accumulate", test_struts_accumulate},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
