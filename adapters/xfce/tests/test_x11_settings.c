#include <string.h>

#include "check.h"
#include "x11_setting_values.h"

static void test_string_property_keeps_text(void)
{
    GValue value = G_VALUE_INIT;
    g_value_init(&value, G_TYPE_STRING);
    g_value_set_string(&value, "Greybird");
    x11_setting setting;
    CHECK(x11_setting_from_property("/Net/ThemeName", &value, &setting));
    CHECK(strcmp("Net/ThemeName", setting.name) == 0);
    CHECK_INT_EQ(X11_SETTING_STRING, setting.type);
    CHECK(strcmp("Greybird", setting.string) == 0);
    g_value_unset(&value);
}

static void test_boolean_becomes_integer(void)
{
    GValue value = G_VALUE_INIT;
    g_value_init(&value, G_TYPE_BOOLEAN);
    g_value_set_boolean(&value, TRUE);
    x11_setting setting;
    CHECK(x11_setting_from_property("/Gtk/MenuImages", &value, &setting));
    CHECK_INT_EQ(X11_SETTING_INTEGER, setting.type);
    CHECK_INT_EQ(1, setting.integer);
}

static void test_dpi_is_clamped_in_xft_units(void)
{
    GValue value = G_VALUE_INIT;
    g_value_init(&value, G_TYPE_INT);
    g_value_set_int(&value, 1000);
    x11_setting setting;
    CHECK(x11_setting_from_property("/Xft/DPI", &value, &setting));
    CHECK_INT_EQ(500 * 1024, setting.integer);
}

static void test_unset_dpi_is_skipped(void)
{
    GValue value = G_VALUE_INIT;
    g_value_init(&value, G_TYPE_INT);
    g_value_set_int(&value, -1);
    x11_setting setting;
    CHECK(!x11_setting_from_property("/Xft/DPI", &value, &setting));
}

static void test_foreign_branches_are_skipped(void)
{
    GValue value = G_VALUE_INIT;
    g_value_init(&value, G_TYPE_INT);
    g_value_set_int(&value, 3);
    x11_setting setting;
    CHECK(!x11_setting_from_property("/Other/Value", &value, &setting));
}

static void test_unsupported_type_is_skipped(void)
{
    GValue value = G_VALUE_INIT;
    g_value_init(&value, G_TYPE_DOUBLE);
    g_value_set_double(&value, 1.5);
    x11_setting setting;
    CHECK(!x11_setting_from_property("/Gtk/Scale", &value, &setting));
}

int main(void)
{
    const check_case cases[] = {
        {"string_property_keeps_text", test_string_property_keeps_text},
        {"boolean_becomes_integer", test_boolean_becomes_integer},
        {"dpi_is_clamped_in_xft_units", test_dpi_is_clamped_in_xft_units},
        {"unset_dpi_is_skipped", test_unset_dpi_is_skipped},
        {"foreign_branches_are_skipped", test_foreign_branches_are_skipped},
        {"unsupported_type_is_skipped", test_unsupported_type_is_skipped},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
