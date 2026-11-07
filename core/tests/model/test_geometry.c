#include "check.h"
#include "wm/geometry.h"

static void test_rect_with_zero_or_negative_size_is_empty(void)
{
    CHECK(wm_rect_is_empty(&(wm_rect) {0, 0, 0, 10}));
    CHECK(wm_rect_is_empty(&(wm_rect) {0, 0, 10, -1}));
    CHECK(!wm_rect_is_empty(&(wm_rect) {0, 0, 1, 1}));
}

static void test_contains_excludes_right_and_bottom_edges(void)
{
    wm_rect rect = {10, 20, 30, 40};
    CHECK(wm_rect_contains(&rect, (wm_point) {10, 20}));
    CHECK(wm_rect_contains(&rect, (wm_point) {39, 59}));
    CHECK(!wm_rect_contains(&rect, (wm_point) {40, 30}));
    CHECK(!wm_rect_contains(&rect, (wm_point) {20, 60}));
    CHECK(!wm_rect_contains(&rect, (wm_point) {9, 30}));
}

static void test_empty_rect_contains_nothing(void)
{
    wm_rect rect = {0, 0, 0, 0};
    CHECK(!wm_rect_contains(&rect, (wm_point) {0, 0}));
}

static void test_touching_rects_do_not_intersect(void)
{
    wm_rect a = {0, 0, 10, 10};
    wm_rect touching = {10, 0, 10, 10};
    wm_rect overlapping = {9, 9, 10, 10};
    CHECK(!wm_rect_intersects(&a, &touching));
    CHECK(wm_rect_intersects(&a, &overlapping));
}

static void test_equals_compares_all_fields(void)
{
    wm_rect a = {1, 2, 3, 4};
    CHECK(wm_rect_equals(&a, &(wm_rect) {1, 2, 3, 4}));
    CHECK(!wm_rect_equals(&a, &(wm_rect) {1, 2, 3, 5}));
}

static void test_right_and_bottom_are_exclusive_ends(void)
{
    wm_rect rect = {5, 6, 10, 20};
    CHECK_INT_EQ(15, wm_rect_right(&rect));
    CHECK_INT_EQ(26, wm_rect_bottom(&rect));
}

static void test_overlap_area_is_intersection_size(void)
{
    wm_rect a = {0, 0, 10, 10};
    wm_rect b = {5, 6, 10, 10};
    wm_rect apart = {20, 20, 5, 5};
    CHECK_INT_EQ(20, wm_rect_overlap_area(&a, &b));
    CHECK_INT_EQ(0, wm_rect_overlap_area(&a, &apart));
}

static void test_center_rounds_down(void)
{
    wm_point center = wm_rect_center(&(wm_rect) {10, 20, 5, 4});
    CHECK_INT_EQ(12, center.x);
    CHECK_INT_EQ(22, center.y);
}

static void test_shrink_and_grow_are_inverse(void)
{
    wm_rect rect = {10, 20, 100, 50};
    wm_extents extents = {1, 2, 3, 4};
    wm_rect shrunk = wm_rect_shrink(&rect, &extents);
    CHECK_RECT_EQ(((wm_rect) {11, 23, 97, 43}), shrunk);
    CHECK_RECT_EQ(rect, wm_rect_grow(&shrunk, &extents));
}

static void test_segments_touching_do_not_overlap(void)
{
    CHECK(!wm_segments_overlap((wm_segment) {0, 10}, (wm_segment) {10, 20}));
    CHECK(wm_segments_overlap((wm_segment) {0, 10}, (wm_segment) {9, 20}));
    CHECK(wm_segments_overlap((wm_segment) {5, 6}, (wm_segment) {0, 20}));
    CHECK(!wm_segments_overlap((wm_segment) {0, 0}, (wm_segment) {0, 10}));
}

static void test_clamp_limits_value_to_range(void)
{
    CHECK_INT_EQ(5, wm_clamp(3, 5, 10));
    CHECK_INT_EQ(10, wm_clamp(30, 5, 10));
    CHECK_INT_EQ(7, wm_clamp(7, 5, 10));
}

static void test_clamp_with_inverted_range_prefers_max(void)
{
    CHECK_INT_EQ(5, wm_clamp(7, 10, 5));
}

static void test_intersection_of_overlapping_rects(void)
{
    wm_rect a = {0, 0, 10, 10};
    wm_rect b = {5, 6, 10, 10};
    CHECK_RECT_EQ(((wm_rect) {5, 6, 5, 4}), wm_rect_intersection(&a, &b));
}

static void test_intersection_of_disjoint_rects_is_empty(void)
{
    wm_rect a = {0, 0, 10, 10};
    wm_rect b = {20, 30, 5, 5};
    CHECK_RECT_EQ(((wm_rect) {20, 30, 0, 0}), wm_rect_intersection(&a, &b));
}

static void test_union_covers_both_rects(void)
{
    wm_rect a = {0, 0, 10, 10};
    wm_rect b = {20, -5, 5, 5};
    CHECK_RECT_EQ(((wm_rect) {0, -5, 25, 15}), wm_rect_union(&a, &b));
}

static void test_min_and_max_pick_smaller_and_larger(void)
{
    CHECK_INT_EQ(-3, wm_min(-3, 4));
    CHECK_INT_EQ(4, wm_max(-3, 4));
    CHECK_INT_EQ(2, wm_min(2, 2));
}

static void test_size_equals_compares_both_fields(void)
{
    CHECK(wm_size_equals((wm_size) {3, 4}, (wm_size) {3, 4}));
    CHECK(!wm_size_equals((wm_size) {3, 4}, (wm_size) {3, 5}));
    CHECK(!wm_size_equals((wm_size) {3, 4}, (wm_size) {2, 4}));
}

static void test_size_with_zero_or_negative_side_is_empty(void)
{
    CHECK(wm_size_is_empty((wm_size) {0, 10}));
    CHECK(wm_size_is_empty((wm_size) {10, -1}));
    CHECK(!wm_size_is_empty((wm_size) {1, 1}));
}

static const check_case CASES[] = {
    {"rect_with_zero_or_negative_size_is_empty", test_rect_with_zero_or_negative_size_is_empty},
    {"contains_excludes_right_and_bottom_edges", test_contains_excludes_right_and_bottom_edges},
    {"empty_rect_contains_nothing", test_empty_rect_contains_nothing},
    {"touching_rects_do_not_intersect", test_touching_rects_do_not_intersect},
    {"equals_compares_all_fields", test_equals_compares_all_fields},
    {"right_and_bottom_are_exclusive_ends", test_right_and_bottom_are_exclusive_ends},
    {"overlap_area_is_intersection_size", test_overlap_area_is_intersection_size},
    {"center_rounds_down", test_center_rounds_down},
    {"shrink_and_grow_are_inverse", test_shrink_and_grow_are_inverse},
    {"segments_touching_do_not_overlap", test_segments_touching_do_not_overlap},
    {"clamp_limits_value_to_range", test_clamp_limits_value_to_range},
    {"clamp_with_inverted_range_prefers_max", test_clamp_with_inverted_range_prefers_max},
    {"intersection_of_overlapping_rects", test_intersection_of_overlapping_rects},
    {"intersection_of_disjoint_rects_is_empty", test_intersection_of_disjoint_rects_is_empty},
    {"union_covers_both_rects", test_union_covers_both_rects},
    {"min_and_max_pick_smaller_and_larger", test_min_and_max_pick_smaller_and_larger},
    {"size_equals_compares_both_fields", test_size_equals_compares_both_fields},
    {"size_with_zero_or_negative_side_is_empty", test_size_with_zero_or_negative_side_is_empty},
};

int main(void)
{
    return check_run(CASES, sizeof(CASES) / sizeof(CASES[0]));
}
