#include "check.h"
#include "server/trim_rule.h"

enum {
    MB = 1024 * 1024,
    START_MS = 1000000,
    BUSY_FRAMES = 30,
};

static trim_state released_at_start(size_t before_mb, size_t after_mb, int backoff_ms)
{
    return (trim_state) {
        .released_ms = START_MS,
        .backoff_ms = backoff_ms,
        .bytes_before = before_mb * MB,
        .bytes_after = after_mb * MB,
        .bytes_checked = after_mb * MB,
    };
}

static trim_tick tick_after(int ms, size_t mb, unsigned frames)
{
    return (trim_tick) {START_MS + ms, mb * MB, frames};
}

static void test_first_tick_releases_what_startup_loaded(void)
{
    trim_state state = trim_rule_start(START_MS);
    trim_tick tick = tick_after(500, 80, BUSY_FRAMES);
    trim_plan plan = trim_rule_plan(&state, &tick);
    CHECK_INT_EQ(TRIM_RELEASE, plan.step);
    CHECK_INT_EQ(TRIM_BACKOFF_MIN_MS, plan.backoff_ms);
}

static void test_memory_above_the_old_level_is_released_while_busy(void)
{
    trim_state state = released_at_start(96, 30, TRIM_BACKOFF_MAX_MS);
    trim_tick tick = tick_after(1000, 96 + 16, BUSY_FRAMES);
    trim_plan plan = trim_rule_plan(&state, &tick);
    CHECK_INT_EQ(TRIM_RELEASE, plan.step);
    CHECK_INT_EQ(TRIM_BACKOFF_MIN_MS, plan.backoff_ms);
}

static void test_growth_inside_the_wait_asks_whether_it_is_new(void)
{
    trim_state state = released_at_start(96, 30, TRIM_BACKOFF_MAX_MS);
    trim_tick tick = tick_after(5000, 90, BUSY_FRAMES);
    trim_plan plan = trim_rule_plan(&state, &tick);
    CHECK_INT_EQ(TRIM_CHECK_NEW, plan.step);
    CHECK_INT_EQ(TRIM_BACKOFF_MAX_MS, plan.backoff_ms);
}

static void test_checked_growth_is_left_alone_until_the_wait_ends(void)
{
    trim_state state = released_at_start(96, 30, 8000);
    state.bytes_checked = 90 * MB;
    trim_tick tick = tick_after(5000, 90, 0);
    trim_plan plan = trim_rule_plan(&state, &tick);
    CHECK_INT_EQ(TRIM_WAIT, plan.step);
    CHECK_INT_EQ(3000, plan.wait_ms);
    CHECK_INT_EQ(8000, plan.backoff_ms);
}

static void test_buffers_back_after_the_wait_are_released_with_a_longer_wait(void)
{
    trim_state state = released_at_start(96, 30, 8000);
    state.bytes_checked = 96 * MB;
    trim_tick tick = tick_after(8000, 96, BUSY_FRAMES);
    trim_plan plan = trim_rule_plan(&state, &tick);
    CHECK_INT_EQ(TRIM_RELEASE, plan.step);
    CHECK_INT_EQ(16000, plan.backoff_ms);
}

static void test_wait_stops_growing_at_the_maximum(void)
{
    trim_state state = released_at_start(96, 30, TRIM_BACKOFF_MAX_MS);
    state.bytes_checked = 96 * MB;
    trim_tick tick = tick_after(TRIM_BACKOFF_MAX_MS, 96, BUSY_FRAMES);
    trim_plan plan = trim_rule_plan(&state, &tick);
    CHECK_INT_EQ(TRIM_RELEASE, plan.step);
    CHECK_INT_EQ(TRIM_BACKOFF_MAX_MS, plan.backoff_ms);
}

static void test_busy_screen_checks_again_next_tick(void)
{
    trim_state state = released_at_start(96, 30, TRIM_BACKOFF_MIN_MS);
    trim_tick tick = tick_after(1000, 35, BUSY_FRAMES);
    trim_plan plan = trim_rule_plan(&state, &tick);
    CHECK_INT_EQ(TRIM_WAIT, plan.step);
    CHECK_INT_EQ(TRIM_TICK_MS, plan.wait_ms);
}

static void test_calm_leftover_waits_for_the_routine_release(void)
{
    trim_state state = released_at_start(96, 30, TRIM_BACKOFF_MIN_MS);
    trim_tick tick = tick_after(10000, 35, 0);
    trim_plan plan = trim_rule_plan(&state, &tick);
    CHECK_INT_EQ(TRIM_WAIT, plan.step);
    CHECK_INT_EQ(TRIM_ROUTINE_MS - 10000, plan.wait_ms);
}

static void test_calm_leftover_is_released_after_the_routine_time(void)
{
    trim_state state = released_at_start(96, 30, TRIM_BACKOFF_MIN_MS);
    trim_tick tick = tick_after(TRIM_ROUTINE_MS, 35, 0);
    trim_plan plan = trim_rule_plan(&state, &tick);
    CHECK_INT_EQ(TRIM_RELEASE, plan.step);
    CHECK_INT_EQ(TRIM_BACKOFF_MIN_MS, plan.backoff_ms);
}

static void test_busy_leftover_is_not_released_by_the_routine(void)
{
    trim_state state = released_at_start(96, 30, TRIM_BACKOFF_MIN_MS);
    trim_tick tick = tick_after(TRIM_ROUTINE_MS, 35, BUSY_FRAMES);
    trim_plan plan = trim_rule_plan(&state, &tick);
    CHECK_INT_EQ(TRIM_WAIT, plan.step);
    CHECK_INT_EQ(TRIM_TICK_MS, plan.wait_ms);
}

static void test_calm_with_nothing_held_rests(void)
{
    trim_state state = released_at_start(96, 30, TRIM_BACKOFF_MIN_MS);
    trim_tick tick = tick_after(10000, 30, 0);
    trim_plan plan = trim_rule_plan(&state, &tick);
    CHECK_INT_EQ(TRIM_REST, plan.step);
}

static void test_quiet_wait_resets_the_wait_length(void)
{
    trim_state state = released_at_start(96, 30, 16000);
    trim_tick tick = tick_after(16000, 31, 0);
    trim_plan plan = trim_rule_plan(&state, &tick);
    CHECK_INT_EQ(TRIM_BACKOFF_MIN_MS, plan.backoff_ms);
}

static void test_wait_length_is_kept_while_it_runs(void)
{
    trim_state state = released_at_start(96, 30, 16000);
    trim_tick tick = tick_after(4000, 31, 0);
    trim_plan plan = trim_rule_plan(&state, &tick);
    CHECK_INT_EQ(16000, plan.backoff_ms);
}

int main(void)
{
    const check_case cases[] = {
        {"first_tick_releases_what_startup_loaded", test_first_tick_releases_what_startup_loaded},
        {"memory_above_the_old_level_is_released_while_busy",
         test_memory_above_the_old_level_is_released_while_busy},
        {"growth_inside_the_wait_asks_whether_it_is_new",
         test_growth_inside_the_wait_asks_whether_it_is_new},
        {"checked_growth_is_left_alone_until_the_wait_ends",
         test_checked_growth_is_left_alone_until_the_wait_ends},
        {"buffers_back_after_the_wait_are_released_with_a_longer_wait",
         test_buffers_back_after_the_wait_are_released_with_a_longer_wait},
        {"wait_stops_growing_at_the_maximum", test_wait_stops_growing_at_the_maximum},
        {"busy_screen_checks_again_next_tick", test_busy_screen_checks_again_next_tick},
        {"calm_leftover_waits_for_the_routine_release",
         test_calm_leftover_waits_for_the_routine_release},
        {"calm_leftover_is_released_after_the_routine_time",
         test_calm_leftover_is_released_after_the_routine_time},
        {"busy_leftover_is_not_released_by_the_routine",
         test_busy_leftover_is_not_released_by_the_routine},
        {"calm_with_nothing_held_rests", test_calm_with_nothing_held_rests},
        {"quiet_wait_resets_the_wait_length", test_quiet_wait_resets_the_wait_length},
        {"wait_length_is_kept_while_it_runs", test_wait_length_is_kept_while_it_runs},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
