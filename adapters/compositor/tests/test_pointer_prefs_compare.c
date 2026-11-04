#include <string.h>

#include "check.h"
#include "pointer_prefs_compare.h"

static void test_unset_prefs_are_equal(void)
{
    pointer_prefs a = { 0 };
    pointer_prefs b = { 0 };

    CHECK(pointer_prefs_equal(&a, &b));
}

static void test_unset_differs_from_set(void)
{
    pointer_prefs a = { 0 };
    pointer_prefs b = { .enabled = { .is_set = true, .value = false } };

    CHECK(!pointer_prefs_equal(&a, &b));
}

static void test_ignores_value_of_unset_choice(void)
{
    pointer_prefs a = { .scroll_button = { .is_set = false, .value = 3 } };
    pointer_prefs b = { .scroll_button = { .is_set = false, .value = 7 } };

    CHECK(pointer_prefs_equal(&a, &b));
}

static void test_methods_differ(void)
{
    pointer_prefs a = { .scroll_method = POINTER_SCROLL_METHOD_EDGE };
    pointer_prefs b = { .scroll_method = POINTER_SCROLL_METHOD_TWO_FINGER };

    CHECK(!pointer_prefs_equal(&a, &b));
}

static void test_absolute_mapping_differs(void)
{
    pointer_prefs a = { 0 };
    pointer_prefs b = { 0 };

    strcpy(b.absolute.output_id, "abc");
    CHECK(!pointer_prefs_equal(&a, &b));
    strcpy(a.absolute.output_id, "abc");
    a.absolute.calibration.is_set = true;
    CHECK(!pointer_prefs_equal(&a, &b));
    b.absolute.calibration.is_set = true;
    b.absolute.calibration.matrix[2] = 0.5f;
    CHECK(!pointer_prefs_equal(&a, &b));
}

int main(void)
{
    static const check_case cases[] = {
        { "unset_prefs_are_equal", test_unset_prefs_are_equal },
        { "unset_differs_from_set", test_unset_differs_from_set },
        { "ignores_value_of_unset_choice", test_ignores_value_of_unset_choice },
        { "methods_differ", test_methods_differ },
        { "absolute_mapping_differs", test_absolute_mapping_differs },
    };

    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
