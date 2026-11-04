#include "check.h"
#include "present_rules.h"

enum {
    REFRESH_NS = 16000000,
    LAST_NS = 1000000000,
};

static const vblank_clock steady = {LAST_NS, REFRESH_NS, true};
static const vblank_clock unknown = {LAST_NS, 0, false};

static void test_fifo_idle_surface_does_not_wait(void)
{
    fifo_counters counters = {false, 3, 3};
    CHECK(!fifo_must_wait(&counters));
}

static void test_fifo_set_barrier_makes_commit_wait(void)
{
    fifo_counters counters = {true, 3, 3};
    CHECK(fifo_must_wait(&counters));
}

static void test_fifo_unapplied_barrier_makes_commit_wait(void)
{
    fifo_counters counters = {false, 4, 3};
    CHECK(fifo_must_wait(&counters));
}

static void test_fifo_waiter_ready_after_clear(void)
{
    fifo_counters counters = {false, 4, 4};
    CHECK(fifo_waiter_ready(&counters, 4));
}

static void test_fifo_waiter_blocked_by_barrier(void)
{
    fifo_counters counters = {true, 4, 4};
    CHECK(!fifo_waiter_ready(&counters, 4));
}

static void test_fifo_waiter_blocked_by_earlier_barrier_commit(void)
{
    fifo_counters counters = {false, 5, 4};
    CHECK(!fifo_waiter_ready(&counters, 5));
    CHECK(fifo_waiter_ready(&counters, 4));
}

static void test_fifo_counters_wrap(void)
{
    fifo_counters counters = {false, 2, 1};
    CHECK(fifo_waiter_ready(&counters, UINT32_MAX));
    CHECK(!fifo_waiter_ready(&counters, 2));
}

static void test_vblank_next_is_strictly_after(void)
{
    CHECK_INT_EQ(LAST_NS + REFRESH_NS, vblank_clock_next(&steady, LAST_NS));
    CHECK_INT_EQ(LAST_NS + 2 * REFRESH_NS, vblank_clock_next(&steady, LAST_NS + REFRESH_NS));
    CHECK_INT_EQ(LAST_NS + 3 * REFRESH_NS,
                 vblank_clock_next(&steady, LAST_NS + 2 * REFRESH_NS + 1));
}

static void test_earliest_idle_is_next_vblank(void)
{
    output_moment moment = {LAST_NS + 1000, false};
    CHECK_INT_EQ(LAST_NS + REFRESH_NS, timing_earliest_present(&steady, moment));
}

static void test_earliest_with_queued_frame_is_one_later(void)
{
    output_moment moment = {LAST_NS + 1000, true};
    CHECK_INT_EQ(LAST_NS + 2 * REFRESH_NS, timing_earliest_present(&steady, moment));
}

static void test_earliest_unknown_clock_is_now(void)
{
    output_moment moment = {LAST_NS + 1000, true};
    CHECK_INT_EQ(LAST_NS + 1000, timing_earliest_present(&unknown, moment));
}

static void test_next_check_unknown_clock_is_target(void)
{
    CHECK_INT_EQ(LAST_NS + 50000000, timing_next_check(&unknown, LAST_NS + 50000000, LAST_NS));
}

static void test_next_check_far_target_wakes_two_frames_ahead(void)
{
    int64_t target = LAST_NS + 10 * REFRESH_NS - 5;
    int64_t check = timing_next_check(&steady, target, LAST_NS);
    CHECK(check > LAST_NS + 8 * REFRESH_NS);
    CHECK(check < LAST_NS + 9 * REFRESH_NS);
}

static void test_next_check_near_target_wakes_after_next_vblank(void)
{
    int64_t target = LAST_NS + 2 * REFRESH_NS;
    int64_t check = timing_next_check(&steady, target, LAST_NS + REFRESH_NS / 2);
    CHECK(check > LAST_NS + REFRESH_NS);
    CHECK(check < LAST_NS + 2 * REFRESH_NS);
}

int main(void)
{
    const check_case cases[] = {
        {"fifo_idle_surface_does_not_wait", test_fifo_idle_surface_does_not_wait},
        {"fifo_set_barrier_makes_commit_wait", test_fifo_set_barrier_makes_commit_wait},
        {"fifo_unapplied_barrier_makes_commit_wait",
         test_fifo_unapplied_barrier_makes_commit_wait},
        {"fifo_waiter_ready_after_clear", test_fifo_waiter_ready_after_clear},
        {"fifo_waiter_blocked_by_barrier", test_fifo_waiter_blocked_by_barrier},
        {"fifo_waiter_blocked_by_earlier_barrier_commit",
         test_fifo_waiter_blocked_by_earlier_barrier_commit},
        {"fifo_counters_wrap", test_fifo_counters_wrap},
        {"vblank_next_is_strictly_after", test_vblank_next_is_strictly_after},
        {"earliest_idle_is_next_vblank", test_earliest_idle_is_next_vblank},
        {"earliest_with_queued_frame_is_one_later", test_earliest_with_queued_frame_is_one_later},
        {"earliest_unknown_clock_is_now", test_earliest_unknown_clock_is_now},
        {"next_check_unknown_clock_is_target", test_next_check_unknown_clock_is_target},
        {"next_check_far_target_wakes_two_frames_ahead",
         test_next_check_far_target_wakes_two_frames_ahead},
        {"next_check_near_target_wakes_after_next_vblank",
         test_next_check_near_target_wakes_after_next_vblank},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
