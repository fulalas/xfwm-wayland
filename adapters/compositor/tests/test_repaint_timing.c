#include "check.h"
#include "render/repaint_timing.h"

enum {
    MS = 1000000,
    REFRESH_120HZ = 8333333,
};

static int delay_for(int64_t since_vblank_ns, int64_t budget_ns)
{
    repaint_plan plan = {
        .now_ns = 1000 * (int64_t) MS + since_vblank_ns,
        .vblank = {1000 * (int64_t) MS, REFRESH_120HZ, true},
        .render_budget_ns = budget_ns,
    };
    return repaint_delay_ms(&plan);
}

static void test_waits_until_budget_before_vblank(void)
{
    CHECK_INT_EQ(5, delay_for(0, 3 * MS));
    CHECK_INT_EQ(3, delay_for(2 * MS, 3 * MS));
}

static void test_no_delay_when_budget_fills_frame(void)
{
    CHECK_INT_EQ(0, delay_for(0, 8 * MS));
    CHECK_INT_EQ(0, delay_for(6 * MS, 3 * MS));
}

static void test_predicts_later_vblank(void)
{
    CHECK_INT_EQ(5, delay_for(REFRESH_120HZ, 3 * MS));
}

static void test_vblank_stamped_slightly_ahead(void)
{
    CHECK_INT_EQ(5, delay_for(-MS / 2, 3 * MS));
}

static void test_delay_capped(void)
{
    CHECK_INT_EQ(6, delay_for(0, 0));
}

static void test_no_delay_without_budget_or_steady_clock(void)
{
    repaint_plan plan = {.now_ns = 5 * MS, .vblank = {0, 0, false}, .render_budget_ns = 3 * MS};
    CHECK_INT_EQ(0, delay_for(0, REPAINT_NO_BUDGET));
    CHECK_INT_EQ(0, repaint_delay_ms(&plan));
}

static void test_budget_needs_samples(void)
{
    render_history history = {0};
    render_history_add(&history, MS);
    CHECK(render_history_budget(&history) == REPAINT_NO_BUDGET);
}

static void test_budget_is_high_percentile_plus_margin(void)
{
    render_history history = {0};
    for (int i = 1; i <= 10; i++) {
        render_history_add(&history, (int64_t) i * MS);
    }
    CHECK(render_history_budget(&history) == 11 * (int64_t) MS);
}

static void test_history_keeps_recent_samples(void)
{
    render_history history = {0};
    for (int i = 0; i < RENDER_HISTORY_MAX; i++) {
        render_history_add(&history, 20 * (int64_t) MS);
    }
    for (int i = 0; i < RENDER_HISTORY_MAX; i++) {
        render_history_add(&history, MS);
    }
    CHECK(render_history_budget(&history) == 3 * (int64_t) MS);
}

int main(void)
{
    const check_case cases[] = {
        {"waits_until_budget_before_vblank", test_waits_until_budget_before_vblank},
        {"no_delay_when_budget_fills_frame", test_no_delay_when_budget_fills_frame},
        {"predicts_later_vblank", test_predicts_later_vblank},
        {"vblank_stamped_slightly_ahead", test_vblank_stamped_slightly_ahead},
        {"delay_capped", test_delay_capped},
        {"no_delay_without_budget_or_steady_clock",
         test_no_delay_without_budget_or_steady_clock},
        {"budget_needs_samples", test_budget_needs_samples},
        {"budget_is_high_percentile_plus_margin", test_budget_is_high_percentile_plus_margin},
        {"history_keeps_recent_samples", test_history_keeps_recent_samples},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
