#include "check.h"
#include "decor_fixture.h"

static const wm_pixmap_kind default_order[] = {
    WM_PIXMAP_SIDE,   WM_PIXMAP_SIDE,   WM_PIXMAP_SIDE,   WM_PIXMAP_TITLE,  WM_PIXMAP_TITLE,
    WM_PIXMAP_TITLE,  WM_PIXMAP_TITLE,  WM_PIXMAP_TITLE,  WM_PIXMAP_TITLE,  WM_PIXMAP_TITLE,
    WM_PIXMAP_TITLE,  WM_PIXMAP_TITLE,  WM_PIXMAP_TITLE,  WM_PIXMAP_CORNER, WM_PIXMAP_CORNER,
    WM_PIXMAP_CORNER, WM_PIXMAP_CORNER, WM_PIXMAP_BUTTON, WM_PIXMAP_BUTTON, WM_PIXMAP_BUTTON,
    WM_PIXMAP_BUTTON, WM_PIXMAP_BUTTON,
};

static void set_title_stretch(decor_fixture *fixture)
{
    fixture_set_all_states(fixture->theme.side_stretch[WM_SIDE_TOP], (wm_size) {30, 24});
}

static void test_ops_follow_xfwm4_paint_order(void)
{
    decor_fixture fixture = fixture_create();
    wm_decor_layout layout = fixture_layout(&fixture);
    CHECK_INT_EQ(sizeof default_order / sizeof default_order[0], layout.op_count);
    for (size_t i = 0; i < layout.op_count && i < sizeof default_order / sizeof default_order[0];
         i++) {
        CHECK_INT_EQ(default_order[i], layout.ops[i].pixmap.kind);
    }
}

static void test_sides_tile_from_their_origin(void)
{
    decor_fixture fixture = fixture_create();
    wm_decor_layout layout = fixture_layout(&fixture);
    const wm_paint_op *left = &layout.ops[0];
    const wm_paint_op *bottom = &layout.ops[2];
    CHECK_INT_EQ(WM_SIDE_LEFT, left->pixmap.index);
    CHECK_INT_EQ(WM_FILL_TILE, left->mode);
    CHECK_RECT_EQ(((wm_rect) {0, 20, 4, 98}), left->dest);
    CHECK_INT_EQ(20, left->origin.y);
    CHECK_RECT_EQ(((wm_rect) {204, 20, 4, 98}), layout.ops[1].dest);
    CHECK_RECT_EQ(((wm_rect) {6, 120, 196, 4}), bottom->dest);
}

static void test_title_parts_tile_from_part_start(void)
{
    decor_fixture fixture = fixture_create();
    wm_decor_layout layout = fixture_layout(&fixture);
    const wm_paint_op *middle = fixture_find_op(&layout, WM_PIXMAP_TITLE, 2);
    CHECK_INT_EQ(23, middle->origin.x);
    CHECK_INT_EQ(0, middle->origin.y);
    CHECK_RECT_EQ(((wm_rect) {6, 0, 196, 20}), middle->clip);
}

static void test_top_strip_reuses_title_parts(void)
{
    decor_fixture fixture = fixture_create();
    wm_decor_layout layout = fixture_layout(&fixture);
    const wm_paint_op *strip = &layout.ops[8];
    CHECK_INT_EQ(WM_PIXMAP_TITLE, strip->pixmap.kind);
    CHECK_RECT_EQ(((wm_rect) {6, 0, 14, 3}), strip->dest);
    CHECK_RECT_EQ(((wm_rect) {6, 0, 196, 3}), strip->clip);
}

static void test_top_strip_prefers_top_pixmaps(void)
{
    decor_fixture fixture = fixture_create();
    fixture_set_all_states(fixture.theme.top[2], (wm_size) {5, 3});
    wm_decor_layout layout = fixture_layout(&fixture);
    const wm_paint_op *top = fixture_find_op(&layout, WM_PIXMAP_TOP, 2);
    CHECK(top != NULL);
    CHECK_INT_EQ(1, fixture_count_ops(&layout, WM_PIXMAP_TOP));
}

static void test_top_strip_height_from_top_pixmap(void)
{
    decor_fixture fixture = fixture_create();
    fixture_set_all_states(fixture.theme.top[3], (wm_size) {3, 5});
    wm_decor_layout layout = fixture_layout(&fixture);
    CHECK_INT_EQ(5, fixture_find_op(&layout, WM_PIXMAP_TOP, 3)->clip.height);
}

static void test_top_strip_height_from_border_top(void)
{
    decor_fixture fixture = fixture_create();
    fixture.config.border_top = 4;
    wm_decor_layout layout = fixture_layout(&fixture);
    CHECK_INT_EQ(4, layout.ops[8].clip.height);
}

static void test_tall_text_removes_top_strip(void)
{
    decor_fixture fixture = fixture_create();
    fixture.window.title_text_size.height = 18;
    wm_decor_layout layout = fixture_layout(&fixture);
    CHECK_INT_EQ(5, fixture_count_ops(&layout, WM_PIXMAP_TITLE));
}

static void test_corners_are_drawn_once_at_frame_corners(void)
{
    decor_fixture fixture = fixture_create();
    wm_decor_layout layout = fixture_layout(&fixture);
    CHECK_RECT_EQ(((wm_rect) {0, 0, 6, 20}), layout.ops[13].dest);
    CHECK_RECT_EQ(((wm_rect) {202, 0, 6, 20}), layout.ops[14].dest);
    CHECK_RECT_EQ(((wm_rect) {0, 118, 6, 6}), layout.ops[15].dest);
    CHECK_RECT_EQ(((wm_rect) {202, 118, 6, 6}), layout.ops[16].dest);
    CHECK_INT_EQ(WM_FILL_PLAIN, layout.ops[13].mode);
}

static void test_corner_stretch_replaces_corner(void)
{
    decor_fixture fixture = fixture_create();
    fixture_set_all_states(fixture.theme.corner_stretch[WM_CORNER_BOTTOM_RIGHT],
                           (wm_size) {8, 8});
    wm_decor_layout layout = fixture_layout(&fixture);
    const wm_paint_op *corner = fixture_find_op(&layout, WM_PIXMAP_CORNER_STRETCH,
                                                WM_CORNER_BOTTOM_RIGHT);
    CHECK_RECT_EQ(((wm_rect) {200, 116, 8, 8}), corner->dest);
}

static void test_side_stretch_scales_along_the_side(void)
{
    decor_fixture fixture = fixture_create();
    fixture_set_all_states(fixture.theme.side_stretch[WM_SIDE_LEFT], (wm_size) {4, 30});
    fixture_set_all_states(fixture.theme.side_stretch[WM_SIDE_BOTTOM], (wm_size) {30, 4});
    wm_decor_layout layout = fixture_layout(&fixture);
    CHECK_INT_EQ(WM_PIXMAP_SIDE_STRETCH, layout.ops[0].pixmap.kind);
    CHECK_INT_EQ(WM_FILL_STRETCH_VERTICAL, layout.ops[0].mode);
    CHECK_INT_EQ(WM_PIXMAP_SIDE, layout.ops[1].pixmap.kind);
    CHECK_INT_EQ(WM_FILL_STRETCH_HORIZONTAL, layout.ops[2].mode);
}

static void test_title_stretch_replaces_title_parts(void)
{
    decor_fixture fixture = fixture_create();
    set_title_stretch(&fixture);
    wm_decor_layout layout = fixture_layout(&fixture);
    const wm_paint_op *title = &layout.ops[3];
    const wm_paint_op *strip = &layout.ops[4];
    CHECK_INT_EQ(0, fixture_count_ops(&layout, WM_PIXMAP_TITLE));
    CHECK_INT_EQ(WM_PIXMAP_SIDE_STRETCH, title->pixmap.kind);
    CHECK_INT_EQ(WM_FILL_STRETCH_HORIZONTAL, title->mode);
    CHECK_RECT_EQ(((wm_rect) {6, 0, 196, 24}), title->dest);
    CHECK_RECT_EQ(((wm_rect) {6, 0, 196, 3}), strip->dest);
}

static void test_title_stretch_clips_text_before_right_edge(void)
{
    decor_fixture fixture = fixture_create();
    set_title_stretch(&fixture);
    wm_text_op text = fixture_layout(&fixture).title_text;
    CHECK(text.visible);
    CHECK_INT_EQ(55, text.position.x);
    CHECK_INT_EQ(7, text.position.y);
    CHECK_RECT_EQ(((wm_rect) {6, 0, 131, 24}), text.clip);
}

static void test_shaded_frame_skips_left_and_right(void)
{
    decor_fixture fixture = fixture_create();
    fixture.window.shaded = true;
    wm_decor_layout layout = fixture_layout(&fixture);
    CHECK_INT_EQ(1, fixture_count_ops(&layout, WM_PIXMAP_SIDE));
    CHECK_INT_EQ(WM_SIDE_BOTTOM, layout.ops[0].pixmap.index);
    CHECK_RECT_EQ(((wm_rect) {6, 20, 196, 4}), layout.ops[0].dest);
}

static void test_shaded_frame_trims_corners_taller_than_frame(void)
{
    decor_fixture fixture = fixture_create();
    fixture.window.shaded = true;
    fixture_set_all_states(fixture.theme.corner[WM_CORNER_TOP_LEFT], (wm_size) {6, 26});
    wm_decor_layout layout = fixture_layout(&fixture);
    const wm_paint_op *top_left = fixture_find_op(&layout, WM_PIXMAP_CORNER, WM_CORNER_TOP_LEFT);
    const wm_paint_op *bottom_left = fixture_find_op(&layout, WM_PIXMAP_CORNER,
                                                     WM_CORNER_BOTTOM_LEFT);
    CHECK_RECT_EQ(((wm_rect) {0, 0, 6, 26}), top_left->dest);
    CHECK_RECT_EQ(((wm_rect) {0, 0, 6, 21}), top_left->clip);
    CHECK_RECT_EQ(((wm_rect) {0, 19, 6, 5}), bottom_left->clip);
}

static void test_maximized_borderless_has_no_sides_or_corners(void)
{
    decor_fixture fixture = fixture_create();
    fixture_maximize(&fixture);
    wm_decor_layout layout = fixture_layout(&fixture);
    CHECK_INT_EQ(0, fixture_count_ops(&layout, WM_PIXMAP_SIDE));
    CHECK_INT_EQ(0, fixture_count_ops(&layout, WM_PIXMAP_CORNER));
    CHECK_INT_EQ(5, fixture_count_ops(&layout, WM_PIXMAP_TITLE));
    CHECK_INT_EQ(WM_PIXMAP_TITLE, layout.ops[0].pixmap.kind);
}

static void test_app_icon_is_painted_over_menu_button(void)
{
    decor_fixture fixture = fixture_create();
    fixture.window.has_icon = true;
    fixture.theme.button[WM_BUTTON_MENU][WM_IMAGE_ACTIVE] = (wm_size) {18, 16};
    wm_decor_layout layout = fixture_layout(&fixture);
    const wm_paint_op *icon = &layout.ops[layout.op_count - 1];
    CHECK(fixture_find_op(&layout, WM_PIXMAP_BUTTON, WM_BUTTON_MENU) != NULL);
    CHECK_INT_EQ(WM_PIXMAP_APP_ICON, icon->pixmap.kind);
    CHECK_RECT_EQ(((wm_rect) {5, 2, 16, 16}), icon->dest);
}

static void test_no_app_icon_without_menu_pixmap(void)
{
    decor_fixture fixture = fixture_create();
    fixture.window.has_icon = true;
    fixture.theme.button[WM_BUTTON_MENU][WM_IMAGE_ACTIVE] = (wm_size) {0, 0};
    wm_decor_layout layout = fixture_layout(&fixture);
    CHECK(fixture_find_op(&layout, WM_PIXMAP_APP_ICON, 0) == NULL);
}

static void test_app_icon_on_stretched_title_uses_title_slice(void)
{
    decor_fixture fixture = fixture_create();
    set_title_stretch(&fixture);
    fixture.window.has_icon = true;
    wm_decor_layout layout = fixture_layout(&fixture);
    const wm_paint_op *slice = &layout.ops[layout.op_count - 6];
    CHECK(fixture_find_op(&layout, WM_PIXMAP_BUTTON, WM_BUTTON_MENU) == NULL);
    CHECK_INT_EQ(WM_PIXMAP_SIDE_STRETCH, slice->pixmap.kind);
    CHECK_RECT_EQ(((wm_rect) {4, 4, 196, 16}), slice->dest);
    CHECK_RECT_EQ(((wm_rect) {4, 4, 16, 16}), slice->clip);
    CHECK_INT_EQ(WM_PIXMAP_APP_ICON, layout.ops[layout.op_count - 1].pixmap.kind);
}

static void test_right_menu_slice_is_offset_from_title_end(void)
{
    decor_fixture fixture = fixture_create();
    set_title_stretch(&fixture);
    fixture.window.has_icon = true;
    fixture_set_layout(&fixture, "|O");
    wm_decor_layout layout = fixture_layout(&fixture);
    const wm_paint_op *slice = &layout.ops[layout.op_count - 2];
    CHECK_INT_EQ(WM_PIXMAP_SIDE_STRETCH, slice->pixmap.kind);
    CHECK_INT_EQ(8, slice->origin.x);
}

int main(void)
{
    static const check_case cases[] = {
        {"ops_follow_xfwm4_paint_order", test_ops_follow_xfwm4_paint_order},
        {"sides_tile_from_their_origin", test_sides_tile_from_their_origin},
        {"title_parts_tile_from_part_start", test_title_parts_tile_from_part_start},
        {"top_strip_reuses_title_parts", test_top_strip_reuses_title_parts},
        {"top_strip_prefers_top_pixmaps", test_top_strip_prefers_top_pixmaps},
        {"top_strip_height_from_top_pixmap", test_top_strip_height_from_top_pixmap},
        {"top_strip_height_from_border_top", test_top_strip_height_from_border_top},
        {"tall_text_removes_top_strip", test_tall_text_removes_top_strip},
        {"corners_are_drawn_once_at_frame_corners", test_corners_are_drawn_once_at_frame_corners},
        {"corner_stretch_replaces_corner", test_corner_stretch_replaces_corner},
        {"side_stretch_scales_along_the_side", test_side_stretch_scales_along_the_side},
        {"title_stretch_replaces_title_parts", test_title_stretch_replaces_title_parts},
        {"title_stretch_clips_text_before_right_edge",
         test_title_stretch_clips_text_before_right_edge},
        {"shaded_frame_skips_left_and_right", test_shaded_frame_skips_left_and_right},
        {"shaded_frame_trims_corners_taller_than_frame",
         test_shaded_frame_trims_corners_taller_than_frame},
        {"maximized_borderless_has_no_sides_or_corners",
         test_maximized_borderless_has_no_sides_or_corners},
        {"app_icon_is_painted_over_menu_button", test_app_icon_is_painted_over_menu_button},
        {"no_app_icon_without_menu_pixmap", test_no_app_icon_without_menu_pixmap},
        {"app_icon_on_stretched_title_uses_title_slice",
         test_app_icon_on_stretched_title_uses_title_slice},
        {"right_menu_slice_is_offset_from_title_end",
         test_right_menu_slice_is_offset_from_title_end},
    };
    return check_run(cases, sizeof cases / sizeof cases[0]);
}
