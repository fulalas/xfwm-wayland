#include "check.h"
#include "shadow_layout.h"

static const shadow_geometry image = {{25, 25}, 12};

static void test_corners_keep_template_corners(void)
{
    wm_rect rect = {10, 20, 200, 100};
    shadow_pieces pieces = shadow_layout_compute(&rect, &image);
    CHECK_RECT_EQ(((wm_rect){0, 0, 12, 12}), pieces.pieces[SHADOW_TOP_LEFT].source);
    CHECK_RECT_EQ(((wm_rect){10, 20, 12, 12}), pieces.pieces[SHADOW_TOP_LEFT].dest);
    CHECK_RECT_EQ(((wm_rect){13, 13, 12, 12}), pieces.pieces[SHADOW_BOTTOM_RIGHT].source);
    CHECK_RECT_EQ(((wm_rect){198, 108, 12, 12}), pieces.pieces[SHADOW_BOTTOM_RIGHT].dest);
}

static void test_edges_stretch_between_corners(void)
{
    wm_rect rect = {0, 0, 200, 100};
    shadow_pieces pieces = shadow_layout_compute(&rect, &image);
    CHECK_RECT_EQ(((wm_rect){12, 0, 1, 12}), pieces.pieces[SHADOW_TOP].source);
    CHECK_RECT_EQ(((wm_rect){12, 0, 176, 12}), pieces.pieces[SHADOW_TOP].dest);
    CHECK_RECT_EQ(((wm_rect){0, 12, 12, 76}), pieces.pieces[SHADOW_LEFT].dest);
}

static void test_middle_fills_the_inside(void)
{
    wm_rect rect = {5, 5, 100, 60};
    shadow_pieces pieces = shadow_layout_compute(&rect, &image);
    CHECK_RECT_EQ(((wm_rect){12, 12, 1, 1}), pieces.pieces[SHADOW_MIDDLE].source);
    CHECK_RECT_EQ(((wm_rect){17, 17, 76, 36}), pieces.pieces[SHADOW_MIDDLE].dest);
}

static void test_small_rect_shrinks_corners(void)
{
    wm_rect rect = {0, 0, 10, 30};
    shadow_pieces pieces = shadow_layout_compute(&rect, &image);
    CHECK_INT_EQ(5, pieces.pieces[SHADOW_TOP_LEFT].dest.width);
    CHECK_INT_EQ(0, pieces.pieces[SHADOW_TOP].dest.width);
    CHECK_INT_EQ(5, pieces.pieces[SHADOW_TOP_RIGHT].dest.x);
}

int main(void)
{
    const check_case cases[] = {
        {"corners_keep_template_corners", test_corners_keep_template_corners},
        {"edges_stretch_between_corners", test_edges_stretch_between_corners},
        {"middle_fills_the_inside", test_middle_fills_the_inside},
        {"small_rect_shrinks_corners", test_small_rect_shrinks_corners},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
