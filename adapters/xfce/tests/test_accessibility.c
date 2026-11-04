#include "accessibility_resolve.h"
#include "check.h"
#include "fake_source.h"

static void test_find_cursor_is_on_by_default(void)
{
    static const fake_entry entries[] = {{NULL, NULL}};
    setting_source source = fake_source(entries);
    CHECK(accessibility_prefs_from(&source).find_cursor);
}

static void test_find_cursor_can_be_turned_off(void)
{
    static const fake_entry entries[] = {{"FindCursor", "false"}, {NULL, NULL}};
    setting_source source = fake_source(entries);
    CHECK(!accessibility_prefs_from(&source).find_cursor);
}

static void test_find_cursor_ignores_unreadable_value(void)
{
    static const fake_entry entries[] = {{"FindCursor", "maybe"}, {NULL, NULL}};
    setting_source source = fake_source(entries);
    CHECK(accessibility_prefs_from(&source).find_cursor);
}

int main(void)
{
    static const check_case cases[] = {
        {"find_cursor_is_on_by_default", test_find_cursor_is_on_by_default},
        {"find_cursor_can_be_turned_off", test_find_cursor_can_be_turned_off},
        {"find_cursor_ignores_unreadable_value", test_find_cursor_ignores_unreadable_value},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
