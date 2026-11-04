#include "check.h"
#include "decor_fixture.h"

static wm_extents extents_of(const decor_fixture *fixture)
{
    wm_decor_input input = fixture_input(fixture);
    return wm_decor_extents(&input);
}

static void check_extents(wm_extents expected, wm_extents actual)
{
    CHECK_INT_EQ(expected.left, actual.left);
    CHECK_INT_EQ(expected.right, actual.right);
    CHECK_INT_EQ(expected.top, actual.top);
    CHECK_INT_EQ(expected.bottom, actual.bottom);
}

static void test_normal_window_has_theme_borders(void)
{
    decor_fixture fixture = fixture_create();
    check_extents((wm_extents) {4, 4, 20, 4}, extents_of(&fixture));
}

static void test_title_height_comes_from_middle_title_part(void)
{
    decor_fixture fixture = fixture_create();
    CHECK_INT_EQ(FIXTURE_TITLE_HEIGHT, wm_decor_title_height(&fixture.theme));
}

static void test_title_height_prefers_title_stretch(void)
{
    decor_fixture fixture = fixture_create();
    fixture.theme.side_stretch[WM_SIDE_TOP][WM_FRAME_ACTIVE] = (wm_size) {30, 24};
    CHECK_INT_EQ(24, wm_decor_title_height(&fixture.theme));
}

static void test_side_stretch_sets_border_width(void)
{
    decor_fixture fixture = fixture_create();
    fixture.theme.side_stretch[WM_SIDE_LEFT][WM_FRAME_ACTIVE] = (wm_size) {7, 30};
    CHECK_INT_EQ(7, extents_of(&fixture).left);
}

static void test_maximized_borderless_keeps_only_title(void)
{
    decor_fixture fixture = fixture_create();
    fixture_maximize(&fixture);
    check_extents((wm_extents) {0, 0, 20, 0}, extents_of(&fixture));
}

static void test_maximized_borderless_title_loses_border_top(void)
{
    decor_fixture fixture = fixture_create();
    fixture_maximize(&fixture);
    fixture.config.border_top = 3;
    CHECK_INT_EQ(17, extents_of(&fixture).top);
}

static void test_half_maximized_keeps_borders(void)
{
    decor_fixture fixture = fixture_create();
    fixture.window.maximized_vertically = true;
    check_extents((wm_extents) {4, 4, 20, 4}, extents_of(&fixture));
}

static void test_maximized_without_borderless_keeps_borders(void)
{
    decor_fixture fixture = fixture_create();
    fixture_maximize(&fixture);
    fixture.config.borderless_maximize = false;
    fixture.config.border_top = 3;
    check_extents((wm_extents) {4, 4, 20, 4}, extents_of(&fixture));
}

static void test_fullscreen_has_no_frame(void)
{
    decor_fixture fixture = fixture_create();
    fixture.window.fullscreen = true;
    wm_decor_layout layout = fixture_layout(&fixture);
    check_extents((wm_extents) {0, 0, 0, 0}, layout.extents);
    CHECK(!layout.has_frame);
    CHECK_INT_EQ(0, layout.op_count);
    CHECK_INT_EQ(0, layout.part_count);
    CHECK_INT_EQ(FIXTURE_CLIENT_HEIGHT, layout.frame_size.height);
}

static void test_borderless_window_has_no_frame(void)
{
    decor_fixture fixture = fixture_create();
    fixture.window.has_border = false;
    wm_decor_layout layout = fixture_layout(&fixture);
    check_extents((wm_extents) {0, 0, 0, 0}, layout.extents);
    CHECK(!layout.has_frame);
}

static void test_titleless_maximize_removes_frame(void)
{
    decor_fixture fixture = fixture_create();
    fixture_maximize(&fixture);
    fixture.config.titleless_maximize = true;
    wm_decor_layout layout = fixture_layout(&fixture);
    CHECK(!layout.has_frame);
    check_extents((wm_extents) {0, 0, 0, 0}, layout.extents);
}

static void test_hide_titlebar_hint_removes_frame_when_maximized(void)
{
    decor_fixture fixture = fixture_create();
    fixture_maximize(&fixture);
    fixture.window.hide_titlebar = true;
    CHECK(!fixture_layout(&fixture).has_frame);
}

static void test_shaded_titleless_maximized_keeps_frame(void)
{
    decor_fixture fixture = fixture_create();
    fixture_maximize(&fixture);
    fixture.config.titleless_maximize = true;
    fixture.window.shaded = true;
    CHECK(fixture_layout(&fixture).has_frame);
}

static void test_frame_size_adds_extents(void)
{
    decor_fixture fixture = fixture_create();
    wm_decor_layout layout = fixture_layout(&fixture);
    CHECK(layout.has_frame);
    CHECK_INT_EQ(208, layout.frame_size.width);
    CHECK_INT_EQ(124, layout.frame_size.height);
}

static void test_shaded_frame_is_title_and_bottom_only(void)
{
    decor_fixture fixture = fixture_create();
    fixture.window.shaded = true;
    wm_decor_layout layout = fixture_layout(&fixture);
    CHECK_INT_EQ(208, layout.frame_size.width);
    CHECK_INT_EQ(24, layout.frame_size.height);
    check_extents((wm_extents) {4, 4, 20, 4}, layout.extents);
}

int main(void)
{
    static const check_case cases[] = {
        {"normal_window_has_theme_borders", test_normal_window_has_theme_borders},
        {"title_height_comes_from_middle_title_part",
         test_title_height_comes_from_middle_title_part},
        {"title_height_prefers_title_stretch", test_title_height_prefers_title_stretch},
        {"side_stretch_sets_border_width", test_side_stretch_sets_border_width},
        {"maximized_borderless_keeps_only_title", test_maximized_borderless_keeps_only_title},
        {"maximized_borderless_title_loses_border_top",
         test_maximized_borderless_title_loses_border_top},
        {"half_maximized_keeps_borders", test_half_maximized_keeps_borders},
        {"maximized_without_borderless_keeps_borders",
         test_maximized_without_borderless_keeps_borders},
        {"fullscreen_has_no_frame", test_fullscreen_has_no_frame},
        {"borderless_window_has_no_frame", test_borderless_window_has_no_frame},
        {"titleless_maximize_removes_frame", test_titleless_maximize_removes_frame},
        {"hide_titlebar_hint_removes_frame_when_maximized",
         test_hide_titlebar_hint_removes_frame_when_maximized},
        {"shaded_titleless_maximized_keeps_frame", test_shaded_titleless_maximized_keeps_frame},
        {"frame_size_adds_extents", test_frame_size_adds_extents},
        {"shaded_frame_is_title_and_bottom_only", test_shaded_frame_is_title_and_bottom_only},
    };
    return check_run(cases, sizeof cases / sizeof cases[0]);
}
