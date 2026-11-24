#include "check.h"
#include "output/primary_rule.h"

static void test_becoming_primary_is_sent_to_every_client(void)
{
    CHECK_INT_EQ(PRIMARY_EVENT_GAINED, primary_rule_event(false, true, true));
    CHECK_INT_EQ(PRIMARY_EVENT_GAINED, primary_rule_event(false, true, false));
}

static void test_losing_primary_is_sent_to_new_clients(void)
{
    CHECK_INT_EQ(PRIMARY_EVENT_LOST, primary_rule_event(true, false, true));
}

static void test_losing_primary_is_not_sent_to_old_clients(void)
{
    CHECK_INT_EQ(PRIMARY_EVENT_NONE, primary_rule_event(true, false, false));
}

static void test_unchanged_state_sends_nothing(void)
{
    CHECK_INT_EQ(PRIMARY_EVENT_NONE, primary_rule_event(true, true, true));
    CHECK_INT_EQ(PRIMARY_EVENT_NONE, primary_rule_event(false, false, true));
    CHECK_INT_EQ(PRIMARY_EVENT_NONE, primary_rule_event(false, false, false));
}

int main(void)
{
    const check_case cases[] = {
        {"becoming_primary_is_sent_to_every_client", test_becoming_primary_is_sent_to_every_client},
        {"losing_primary_is_sent_to_new_clients", test_losing_primary_is_sent_to_new_clients},
        {"losing_primary_is_not_sent_to_old_clients", test_losing_primary_is_not_sent_to_old_clients},
        {"unchanged_state_sends_nothing", test_unchanged_state_sends_nothing},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
