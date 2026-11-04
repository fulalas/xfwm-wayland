#include "check.h"
#include "device_settings.h"

static void test_choice_orders_words(void)
{
    device_setting setting = device_setting_choice(6, 2, 4);

    CHECK(setting.available);
    CHECK_INT_EQ(6, setting.words[0]);
    CHECK_INT_EQ(2, setting.words[1]);
    CHECK_INT_EQ(4, setting.words[2]);
}

static void test_pair_orders_words(void)
{
    device_setting setting = device_setting_pair(1, 0);

    CHECK(setting.available);
    CHECK_INT_EQ(1, setting.words[0]);
    CHECK_INT_EQ(0, setting.words[1]);
}

static void test_equal_settings(void)
{
    device_setting a = device_setting_pair(0, 1);
    device_setting b = device_setting_pair(0, 1);

    CHECK(device_setting_equal(&a, &b));
}

static void test_current_value_change_is_detected(void)
{
    device_setting a = device_setting_pair(0, 1);
    device_setting b = device_setting_pair(0, 0);

    CHECK(!device_setting_equal(&a, &b));
}

static void test_number_change_is_detected(void)
{
    device_setting a = { .available = true, .numbers = { 0.0, 0.5 } };
    device_setting b = { .available = true, .numbers = { 0.0, -0.5 } };

    CHECK(!device_setting_equal(&a, &b));
}

static void test_availability_change_is_detected(void)
{
    device_setting missing = { 0 };
    device_setting present = device_setting_pair(0, 0);

    CHECK(!device_setting_equal(&missing, &present));
}

int main(void)
{
    static const check_case cases[] = {
        { "choice_orders_words", test_choice_orders_words },
        { "pair_orders_words", test_pair_orders_words },
        { "equal_settings", test_equal_settings },
        { "current_value_change_is_detected", test_current_value_change_is_detected },
        { "number_change_is_detected", test_number_change_is_detected },
        { "availability_change_is_detected", test_availability_change_is_detected },
    };

    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
