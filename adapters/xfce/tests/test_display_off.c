#include "check.h"
#include "display_off.h"
#include "fake_source.h"

static void test_defaults_follow_the_power_manager(void)
{
    static const fake_entry entries[] = { { NULL, NULL } };
    setting_source source = fake_source(entries);
    CHECK_INT_EQ(600000, display_off_ms_from(&source, false));
    CHECK_INT_EQ(300000, display_off_ms_from(&source, true));
}

static void test_earlier_of_sleep_and_off_wins(void)
{
    static const fake_entry entries[] = {
        { "dpms-on-ac-sleep", "20" }, { "dpms-on-ac-off", "5" },
        { "dpms-on-battery-sleep", "0" }, { "dpms-on-battery-off", "7" }, { NULL, NULL },
    };
    setting_source source = fake_source(entries);
    CHECK_INT_EQ(300000, display_off_ms_from(&source, false));
    CHECK_INT_EQ(420000, display_off_ms_from(&source, true));
}

static void test_never_when_both_are_zero_or_disabled(void)
{
    static const fake_entry zeros[] = {
        { "dpms-on-ac-sleep", "0" }, { "dpms-on-ac-off", "0" }, { NULL, NULL },
    };
    static const fake_entry disabled[] = { { "dpms-enabled", "false" }, { NULL, NULL } };
    static const fake_entry presenting[] = { { "presentation-mode", "true" }, { NULL, NULL } };
    setting_source zero_source = fake_source(zeros);
    setting_source disabled_source = fake_source(disabled);
    setting_source presenting_source = fake_source(presenting);
    CHECK_INT_EQ(0, display_off_ms_from(&zero_source, false));
    CHECK_INT_EQ(0, display_off_ms_from(&disabled_source, false));
    CHECK_INT_EQ(0, display_off_ms_from(&presenting_source, false));
}

static void test_huge_times_are_capped(void)
{
    static const fake_entry entries[] = { { "dpms-on-ac-off", "999999" }, { NULL, NULL } };
    setting_source source = fake_source(entries);
    CHECK_INT_EQ(600000, display_off_ms_from(&source, false));
    static const fake_entry both[] = {
        { "dpms-on-ac-sleep", "999999" }, { "dpms-on-ac-off", "999999" }, { NULL, NULL },
    };
    setting_source capped = fake_source(both);
    CHECK_INT_EQ(2100000000u, display_off_ms_from(&capped, false));
}

int main(void)
{
    static const check_case cases[] = {
        { "defaults_follow_the_power_manager", test_defaults_follow_the_power_manager },
        { "earlier_of_sleep_and_off_wins", test_earlier_of_sleep_and_off_wins },
        { "never_when_both_are_zero_or_disabled", test_never_when_both_are_zero_or_disabled },
        { "huge_times_are_capped", test_huge_times_are_capped },
    };

    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
