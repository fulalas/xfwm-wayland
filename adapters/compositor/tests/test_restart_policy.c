#include "check.h"
#include "server/restart_policy.h"

static void test_first_crash_restarts_after_a_second(void)
{
    restart_policy policy = {0, 0};
    restart_policy_record_crash(&policy, 1000);
    restart_decision decision = restart_policy_next(&policy);
    CHECK(decision.restart);
    CHECK_INT_EQ(1000, decision.delay_ms);
}

static void test_delay_doubles_with_each_crash(void)
{
    restart_policy policy = {0, 0};
    restart_policy_record_crash(&policy, 1000);
    restart_policy_record_crash(&policy, 2000);
    restart_policy_record_crash(&policy, 3000);
    CHECK_INT_EQ(4000, restart_policy_next(&policy).delay_ms);
}

static void test_gives_up_after_five_quick_crashes(void)
{
    restart_policy policy = {0, 0};
    for (int i = 0; i < 5; i++) {
        restart_policy_record_crash(&policy, 1000 + i * 1000);
    }
    CHECK(!restart_policy_next(&policy).restart);
}

static void test_old_crashes_are_forgotten(void)
{
    restart_policy policy = {0, 0};
    for (int i = 0; i < 4; i++) {
        restart_policy_record_crash(&policy, 1000 + i * 1000);
    }
    restart_policy_record_crash(&policy, 1000 + 4 * 60 * 1000);
    restart_decision decision = restart_policy_next(&policy);
    CHECK(decision.restart);
    CHECK_INT_EQ(1000, decision.delay_ms);
}

int main(void)
{
    const check_case cases[] = {
        {"first_crash_restarts_after_a_second", test_first_crash_restarts_after_a_second},
        {"delay_doubles_with_each_crash", test_delay_doubles_with_each_crash},
        {"gives_up_after_five_quick_crashes", test_gives_up_after_five_quick_crashes},
        {"old_crashes_are_forgotten", test_old_crashes_are_forgotten},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
