#include "check.h"
#include "osd/find_cursor_rings.h"

static const double TOLERANCE = 0.01;

static uint32_t elapsed_for_radius(double radius)
{
    double growth = FIND_CURSOR_MAX_RADIUS - FIND_CURSOR_MIN_RADIUS;
    return (uint32_t) ((radius - FIND_CURSOR_MIN_RADIUS) / growth * FIND_CURSOR_DURATION_MS);
}

static void test_starts_as_one_small_ring(void)
{
    find_cursor_frame frame = find_cursor_frame_at(0);
    CHECK_DOUBLE_NEAR(FIND_CURSOR_MIN_RADIUS, frame.radius, TOLERANCE);
    CHECK_INT_EQ(1, frame.rings);
}

static void test_radius_grows_with_time(void)
{
    find_cursor_frame early = find_cursor_frame_at(FIND_CURSOR_DURATION_MS / 4);
    find_cursor_frame late = find_cursor_frame_at(FIND_CURSOR_DURATION_MS / 2);
    CHECK(late.radius > early.radius);
    double middle = (FIND_CURSOR_MIN_RADIUS + FIND_CURSOR_MAX_RADIUS) / 2.0;
    CHECK_DOUBLE_NEAR(middle, late.radius, 1.0);
}

static void test_rings_appear_every_gap(void)
{
    CHECK_INT_EQ(1, find_cursor_frame_at(elapsed_for_radius(25)).rings);
    CHECK_INT_EQ(2, find_cursor_frame_at(elapsed_for_radius(45)).rings);
    CHECK_INT_EQ(3, find_cursor_frame_at(elapsed_for_radius(75)).rings);
    CHECK_INT_EQ(4, find_cursor_frame_at(elapsed_for_radius(120)).rings);
    CHECK_INT_EQ(4, find_cursor_frame_at(elapsed_for_radius(240)).rings);
}

static void test_inner_rings_are_spaced_by_gap(void)
{
    find_cursor_frame frame = find_cursor_frame_at(elapsed_for_radius(120));
    double outer = find_cursor_ring_radius(&frame, 0);
    CHECK_DOUBLE_NEAR(frame.radius, outer, TOLERANCE);
    CHECK_DOUBLE_NEAR(outer - FIND_CURSOR_RING_GAP, find_cursor_ring_radius(&frame, 1), TOLERANCE);
}

static void test_stops_after_duration(void)
{
    CHECK(find_cursor_is_running(FIND_CURSOR_DURATION_MS - 1));
    CHECK(!find_cursor_is_running(FIND_CURSOR_DURATION_MS));
}

static void test_radius_never_exceeds_maximum(void)
{
    find_cursor_frame frame = find_cursor_frame_at(FIND_CURSOR_DURATION_MS * 3);
    CHECK_DOUBLE_NEAR(FIND_CURSOR_MAX_RADIUS, frame.radius, TOLERANCE);
}

static void test_half_size_covers_stroke(void)
{
    find_cursor_frame frame = find_cursor_frame_at(FIND_CURSOR_DURATION_MS / 3);
    int half = find_cursor_half_size(&frame);
    CHECK(half >= frame.radius + FIND_CURSOR_LINE_WIDTH / 2.0);
    CHECK(half <= frame.radius + FIND_CURSOR_LINE_WIDTH + 1);
}

int main(void)
{
    static const check_case cases[] = {
        {"starts_as_one_small_ring", test_starts_as_one_small_ring},
        {"radius_grows_with_time", test_radius_grows_with_time},
        {"rings_appear_every_gap", test_rings_appear_every_gap},
        {"inner_rings_are_spaced_by_gap", test_inner_rings_are_spaced_by_gap},
        {"stops_after_duration", test_stops_after_duration},
        {"radius_never_exceeds_maximum", test_radius_never_exceeds_maximum},
        {"half_size_covers_stroke", test_half_size_covers_stroke},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
