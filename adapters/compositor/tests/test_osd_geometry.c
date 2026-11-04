#include "check.h"
#include "osd_geometry.h"

static const wm_rect monitor = {0, 0, 1920, 1080};

static void test_menu_inside_area_keeps_anchor(void)
{
    wm_point origin = osd_clamp_origin((wm_point){100, 200}, (wm_size){150, 300}, &monitor);
    CHECK_INT_EQ(100, origin.x);
    CHECK_INT_EQ(200, origin.y);
}

static void test_menu_near_edge_is_pushed_back(void)
{
    wm_point origin = osd_clamp_origin((wm_point){1900, 1000}, (wm_size){150, 300}, &monitor);
    CHECK_INT_EQ(1770, origin.x);
    CHECK_INT_EQ(780, origin.y);
}

static void test_menu_larger_than_area_starts_at_area(void)
{
    wm_point origin = osd_clamp_origin((wm_point){50, 50}, (wm_size){2000, 100}, &monitor);
    CHECK_INT_EQ(0, origin.x);
}

static void test_submenu_opens_right_of_item(void)
{
    wm_rect row = {100, 240, 200, 0};
    wm_point origin = osd_submenu_origin(&row, (wm_size){150, 300}, &monitor);
    CHECK_INT_EQ(300, origin.x);
    CHECK_INT_EQ(240, origin.y);
}

static void test_submenu_flips_left_at_right_edge(void)
{
    wm_rect row = {1700, 100, 200, 0};
    wm_point origin = osd_submenu_origin(&row, (wm_size){150, 300}, &monitor);
    CHECK_INT_EQ(1550, origin.x);
}

static void test_submenu_is_pushed_up_at_bottom_edge(void)
{
    wm_rect row = {100, 1000, 200, 0};
    wm_point origin = osd_submenu_origin(&row, (wm_size){150, 300}, &monitor);
    CHECK_INT_EQ(780, origin.y);
}

static void test_item_row_spans_panel_at_item_height(void)
{
    wm_rect panel = {100, 200, 180, 400};
    wm_rect item = {1, 53, 178, 24};
    CHECK_RECT_EQ(((wm_rect){100, 253, 180, 24}), osd_item_row(&panel, &item));
}

static void test_submenu_aligns_with_item_rect(void)
{
    wm_rect panel = {100, 200, 180, 400};
    wm_rect item = {1, 53, 178, 24};
    wm_rect row = osd_item_row(&panel, &item);
    wm_point origin = osd_submenu_origin(&row, (wm_size){150, 300}, &monitor);
    CHECK_INT_EQ(280, origin.x);
    CHECK_INT_EQ(253, origin.y);
}

static void test_submenu_from_item_rect_flips_left(void)
{
    wm_rect panel = {1700, 200, 180, 400};
    wm_rect item = {1, 53, 178, 24};
    wm_rect row = osd_item_row(&panel, &item);
    wm_point origin = osd_submenu_origin(&row, (wm_size){150, 300}, &monitor);
    CHECK_INT_EQ(1550, origin.x);
    CHECK_INT_EQ(253, origin.y);
}

static void test_item_at_finds_containing_rect(void)
{
    const wm_rect items[] = {{1, 5, 100, 24}, {1, 29, 100, 7}, {1, 36, 100, 24}};
    CHECK_INT_EQ(0, osd_item_at(items, 3, (wm_point){50, 5}));
    CHECK_INT_EQ(1, osd_item_at(items, 3, (wm_point){50, 30}));
    CHECK_INT_EQ(2, osd_item_at(items, 3, (wm_point){50, 59}));
}

static void test_item_at_misses_outside_rects(void)
{
    const wm_rect items[] = {{1, 5, 100, 24}, {1, 29, 100, 24}};
    CHECK_INT_EQ(-1, osd_item_at(items, 2, (wm_point){0, 10}));
    CHECK_INT_EQ(-1, osd_item_at(items, 2, (wm_point){50, 53}));
    CHECK_INT_EQ(-1, osd_item_at(items, 1, (wm_point){50, 30}));
}

static void test_cycle_is_centered(void)
{
    wm_rect second = {1920, 0, 1280, 1024};
    wm_point origin = osd_center_origin((wm_size){400, 200}, &second);
    CHECK_INT_EQ(2360, origin.x);
    CHECK_INT_EQ(412, origin.y);
}

static void test_preview_keeps_aspect_ratio(void)
{
    wm_rect slot = {10, 10, 100, 100};
    CHECK_RECT_EQ(((wm_rect){10, 35, 100, 50}),
                      osd_fit_preview((wm_size){400, 200}, &slot));
}

static void test_small_preview_is_not_enlarged(void)
{
    wm_rect slot = {0, 0, 100, 100};
    CHECK_RECT_EQ(((wm_rect){40, 30, 20, 40}), osd_fit_preview((wm_size){20, 40}, &slot));
}

int main(void)
{
    const check_case cases[] = {
        {"menu_inside_area_keeps_anchor", test_menu_inside_area_keeps_anchor},
        {"submenu_opens_right_of_item", test_submenu_opens_right_of_item},
        {"submenu_flips_left_at_right_edge", test_submenu_flips_left_at_right_edge},
        {"submenu_is_pushed_up_at_bottom_edge", test_submenu_is_pushed_up_at_bottom_edge},
        {"menu_near_edge_is_pushed_back", test_menu_near_edge_is_pushed_back},
        {"menu_larger_than_area_starts_at_area", test_menu_larger_than_area_starts_at_area},
        {"item_row_spans_panel_at_item_height", test_item_row_spans_panel_at_item_height},
        {"submenu_aligns_with_item_rect", test_submenu_aligns_with_item_rect},
        {"submenu_from_item_rect_flips_left", test_submenu_from_item_rect_flips_left},
        {"item_at_finds_containing_rect", test_item_at_finds_containing_rect},
        {"item_at_misses_outside_rects", test_item_at_misses_outside_rects},
        {"cycle_is_centered", test_cycle_is_centered},
        {"preview_keeps_aspect_ratio", test_preview_keeps_aspect_ratio},
        {"small_preview_is_not_enlarged", test_small_preview_is_not_enlarged},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
