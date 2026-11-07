#include "check.h"
#include "decor_fixture.h"

static decor_fixture aligned_fixture(wm_alignment alignment, bool full_width)
{
    decor_fixture fixture = fixture_create();
    fixture.config.title.alignment = alignment;
    fixture.config.title.full_width = full_width;
    return fixture;
}

static int text_x(const decor_fixture *fixture)
{
    return fixture_layout(fixture).title_text.position.x;
}

static wm_rect title_part_dest(const decor_fixture *fixture, int part)
{
    wm_decor_layout layout = fixture_layout(fixture);
    const wm_paint_op *op = fixture_find_op(&layout, WM_PIXMAP_TITLE, part);
    return op == NULL ? (wm_rect) {0, 0, 0, 0} : op->dest;
}

static void test_full_width_center_text_position(void)
{
    decor_fixture fixture = aligned_fixture(WM_ALIGN_CENTER, true);
    wm_text_op text = fixture_layout(&fixture).title_text;
    CHECK(text.visible);
    CHECK_INT_EQ(55, text.position.x);
    CHECK_INT_EQ(5, text.position.y);
}

static void test_full_width_middle_spans_between_buttons(void)
{
    decor_fixture fixture = aligned_fixture(WM_ALIGN_CENTER, true);
    CHECK_RECT_EQ(((wm_rect) {6, 0, 14, 20}), title_part_dest(&fixture, 0));
    CHECK_RECT_EQ(((wm_rect) {20, 0, 3, 20}), title_part_dest(&fixture, 1));
    CHECK_RECT_EQ(((wm_rect) {23, 0, 114, 20}), title_part_dest(&fixture, 2));
    CHECK_RECT_EQ(((wm_rect) {137, 0, 3, 20}), title_part_dest(&fixture, 3));
    CHECK_RECT_EQ(((wm_rect) {140, 0, 62, 20}), title_part_dest(&fixture, 4));
}

static void test_full_width_left_and_right_alignment(void)
{
    decor_fixture left = aligned_fixture(WM_ALIGN_LEFT, true);
    decor_fixture right = aligned_fixture(WM_ALIGN_RIGHT, true);
    CHECK_INT_EQ(23, text_x(&left));
    CHECK_INT_EQ(87, text_x(&right));
}

static void test_full_width_horizontal_offset(void)
{
    decor_fixture left = aligned_fixture(WM_ALIGN_LEFT, true);
    decor_fixture right = aligned_fixture(WM_ALIGN_RIGHT, true);
    left.config.title.horizontal_offset = 5;
    right.config.title.horizontal_offset = 5;
    CHECK_INT_EQ(28, text_x(&left));
    CHECK_INT_EQ(82, text_x(&right));
}

static void test_full_width_long_text_starts_at_offset(void)
{
    decor_fixture fixture = aligned_fixture(WM_ALIGN_CENTER, true);
    fixture.window.title_text_size.width = 200;
    CHECK_INT_EQ(23, text_x(&fixture));
}

static void test_fitted_title_middle_matches_text_width(void)
{
    decor_fixture fixture = aligned_fixture(WM_ALIGN_CENTER, false);
    CHECK_RECT_EQ(((wm_rect) {52, 0, 3, 20}), title_part_dest(&fixture, 1));
    CHECK_RECT_EQ(((wm_rect) {55, 0, 50, 20}), title_part_dest(&fixture, 2));
    CHECK_RECT_EQ(((wm_rect) {105, 0, 3, 20}), title_part_dest(&fixture, 3));
    CHECK_RECT_EQ(((wm_rect) {108, 0, 196, 20}), title_part_dest(&fixture, 4));
    CHECK_INT_EQ(55, text_x(&fixture));
}

static void test_fitted_left_and_right_alignment(void)
{
    decor_fixture left = aligned_fixture(WM_ALIGN_LEFT, false);
    decor_fixture right = aligned_fixture(WM_ALIGN_RIGHT, false);
    CHECK_INT_EQ(23, text_x(&left));
    CHECK_INT_EQ(87, text_x(&right));
}

static void test_fitted_title_adds_shadow_width(void)
{
    decor_fixture fixture = aligned_fixture(WM_ALIGN_CENTER, false);
    fixture.config.title.shadow_active = WM_TITLE_SHADOW_FRAME;
    CHECK_INT_EQ(52, title_part_dest(&fixture, 2).width);
}

static void test_fitted_long_text_is_limited_to_title_width(void)
{
    decor_fixture fixture = aligned_fixture(WM_ALIGN_CENTER, false);
    fixture.window.title_text_size.width = 500;
    CHECK_RECT_EQ(((wm_rect) {23, 0, 190, 20}), title_part_dest(&fixture, 2));
}

static void test_fitted_empty_text_is_not_drawn(void)
{
    decor_fixture fixture = aligned_fixture(WM_ALIGN_CENTER, false);
    fixture.window.title_text_size.width = 0;
    CHECK(!fixture_layout(&fixture).title_text.visible);
}

static void test_vertical_offset_moves_text_down(void)
{
    decor_fixture fixture = fixture_create();
    fixture.config.title.vertical_offset_active = 3;
    CHECK_INT_EQ(8, fixture_layout(&fixture).title_text.position.y);
}

static void test_vertical_offset_cannot_push_text_out(void)
{
    decor_fixture fixture = fixture_create();
    fixture.config.title.vertical_offset_active = 15;
    CHECK_INT_EQ(10, fixture_layout(&fixture).title_text.position.y);
}

static void test_inactive_title_uses_inactive_settings(void)
{
    decor_fixture fixture = fixture_create();
    fixture.window.state = WM_FRAME_INACTIVE;
    fixture.config.title.vertical_offset_inactive = 2;
    fixture.config.title.shadow_inactive = WM_TITLE_SHADOW_UNDER;
    wm_text_op text = fixture_layout(&fixture).title_text;
    CHECK_INT_EQ(7, text.position.y);
    CHECK_INT_EQ(WM_FRAME_INACTIVE, text.state);
    CHECK_INT_EQ(WM_TITLE_SHADOW_UNDER, text.shadow);
}

static void test_text_is_clipped_before_right_title_edge(void)
{
    decor_fixture fixture = fixture_create();
    CHECK_RECT_EQ(((wm_rect) {6, 0, 131, 20}), fixture_layout(&fixture).title_text.clip);
}

static int first_button_right_of(const wm_decor_layout *layout, int x)
{
    int first = layout->frame_size.width;
    for (size_t i = 0; i < layout->part_count; i++) {
        const wm_part_rect *part = &layout->parts[i];
        bool is_button = part->part >= WM_PART_BUTTON_FIRST && part->part <= WM_PART_BUTTON_LAST;
        if (is_button && part->rect.x > x && part->rect.x < first) {
            first = part->rect.x;
        }
    }
    return first;
}

static void test_long_title_stops_before_buttons(void)
{
    for (int full_width = 0; full_width <= 1; full_width++) {
        decor_fixture fixture = aligned_fixture(WM_ALIGN_CENTER, full_width);
        fixture.window.client_size.width = 120;
        fixture.window.title_text_size.width = 500;
        wm_decor_layout layout = fixture_layout(&fixture);
        wm_rect clip = layout.title_text.clip;
        CHECK(clip.width > 0);
        CHECK(wm_rect_right(&clip) <= first_button_right_of(&layout, clip.x));
    }
}

static void test_border_top_shifts_title_up(void)
{
    decor_fixture fixture = fixture_create();
    fixture.window.maximized_horizontally = true;
    fixture.window.maximized_vertically = true;
    fixture.config.border_top = 2;
    wm_text_op text = fixture_layout(&fixture).title_text;
    CHECK_INT_EQ(3, text.position.y);
    CHECK_RECT_EQ(((wm_rect) {0, -2, 133, 20}), text.clip);
}

static void test_no_title_when_corners_fill_the_width(void)
{
    decor_fixture fixture = fixture_create();
    fixture.window.client_size.width = 0;
    wm_decor_layout layout = fixture_layout(&fixture);
    CHECK(!layout.title_text.visible);
    CHECK_INT_EQ(0, fixture_count_ops(&layout, WM_PIXMAP_TITLE));
}

static const check_case CASES[] = {
    {"full_width_center_text_position", test_full_width_center_text_position},
    {"full_width_middle_spans_between_buttons", test_full_width_middle_spans_between_buttons},
    {"full_width_left_and_right_alignment", test_full_width_left_and_right_alignment},
    {"full_width_horizontal_offset", test_full_width_horizontal_offset},
    {"full_width_long_text_starts_at_offset", test_full_width_long_text_starts_at_offset},
    {"fitted_title_middle_matches_text_width", test_fitted_title_middle_matches_text_width},
    {"fitted_left_and_right_alignment", test_fitted_left_and_right_alignment},
    {"fitted_title_adds_shadow_width", test_fitted_title_adds_shadow_width},
    {"fitted_long_text_is_limited_to_title_width",
     test_fitted_long_text_is_limited_to_title_width},
    {"fitted_empty_text_is_not_drawn", test_fitted_empty_text_is_not_drawn},
    {"vertical_offset_moves_text_down", test_vertical_offset_moves_text_down},
    {"vertical_offset_cannot_push_text_out", test_vertical_offset_cannot_push_text_out},
    {"inactive_title_uses_inactive_settings", test_inactive_title_uses_inactive_settings},
    {"text_is_clipped_before_right_title_edge", test_text_is_clipped_before_right_title_edge},
    {"long_title_stops_before_buttons", test_long_title_stops_before_buttons},
    {"border_top_shifts_title_up", test_border_top_shifts_title_up},
    {"no_title_when_corners_fill_the_width", test_no_title_when_corners_fill_the_width},
};

int main(void)
{
    return check_run(CASES, sizeof(CASES) / sizeof(CASES[0]));
}
