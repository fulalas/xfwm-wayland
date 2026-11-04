#include <stdlib.h>
#include <string.h>

#include "check.h"
#include "xresources.h"
#include "xsettings_wire.h"

static void add_integer(xsettings_table *table, const char *name, int32_t value)
{
    x11_setting setting = {name, X11_SETTING_INTEGER, value, NULL};
    xsettings_table_add(table, &setting);
}

static void add_string(xsettings_table *table, const char *name, const char *value)
{
    x11_setting setting = {name, X11_SETTING_STRING, 0, value};
    xsettings_table_add(table, &setting);
}

static uint32_t read_u32(const uint8_t *bytes)
{
    uint32_t value;
    memcpy(&value, bytes, sizeof(value));
    return value;
}

static void test_window_scale_is_forced_to_one(void)
{
    xsettings_table *table = xsettings_table_create();
    add_integer(table, "Gdk/WindowScalingFactor", 2);
    add_integer(table, "Gdk/UnscaledDPI", 98304);
    CHECK_INT_EQ(1, xsettings_table_count(table));
    CHECK_INT_EQ(1, xsettings_table_find(table, "Gdk/WindowScalingFactor")->integer);
    CHECK(xsettings_table_find(table, "Gdk/UnscaledDPI") == NULL);
    xsettings_table_destroy(table);
}

static void test_later_value_replaces_earlier(void)
{
    xsettings_table *table = xsettings_table_create();
    add_string(table, "Net/ThemeName", "Adwaita");
    add_string(table, "Net/ThemeName", "Greybird");
    CHECK_INT_EQ(2, xsettings_table_count(table));
    CHECK(strcmp("Greybird", xsettings_table_find(table, "Net/ThemeName")->string) == 0);
    xsettings_table_destroy(table);
}

static void test_reset_keeps_only_window_scale(void)
{
    xsettings_table *table = xsettings_table_create();
    add_string(table, "Net/ThemeName", "Greybird");
    xsettings_table_reset(table);
    CHECK_INT_EQ(1, xsettings_table_count(table));
    xsettings_table_destroy(table);
}

static void test_wire_layout_of_string_setting(void)
{
    xsettings_table *table = xsettings_table_create();
    add_string(table, "Net/ThemeName", "Greybird");
    xsettings_bytes bytes = xsettings_wire_encode(table, 7);
    CHECK_INT_EQ(12 + 36 + 36, bytes.length);
    CHECK_INT_EQ(7, read_u32(bytes.data + 4));
    CHECK_INT_EQ(2, read_u32(bytes.data + 8));
    const uint8_t *second = bytes.data + 12 + 4 + 24 + 4 + 4;
    CHECK_INT_EQ(1, second[0]);
    CHECK_INT_EQ(8, read_u32(second + 4 + 16 + 4));
    CHECK(memcmp(second + 4 + 16 + 4 + 4, "Greybird", 8) == 0);
    xsettings_bytes_free(&bytes);
    xsettings_table_destroy(table);
}

static void test_xresources_replace_managed_lines(void)
{
    xsettings_table *table = xsettings_table_create();
    add_integer(table, "Xft/DPI", 110 * 1024);
    add_integer(table, "Xft/Hinting", -1);
    add_string(table, "Xft/HintStyle", "hintslight");
    char *merged = xresources_merge("XTerm*faceName:\tMono\nXft.dpi:\t96\nXft.hinting: 1\n", table);
    CHECK(strstr(merged, "XTerm*faceName:\tMono\n") != NULL);
    CHECK(strstr(merged, "Xft.dpi:\t110\n") != NULL);
    CHECK(strstr(merged, "96") == NULL);
    CHECK(strstr(merged, "Xft.hinting") == NULL);
    CHECK(strstr(merged, "Xft.hintstyle:\thintslight\n") != NULL);
    CHECK(strstr(merged, "Xcursor.theme_core:\t1\n") != NULL);
    free(merged);
    xsettings_table_destroy(table);
}

int main(void)
{
    const check_case cases[] = {
        {"window_scale_is_forced_to_one", test_window_scale_is_forced_to_one},
        {"later_value_replaces_earlier", test_later_value_replaces_earlier},
        {"reset_keeps_only_window_scale", test_reset_keeps_only_window_scale},
        {"wire_layout_of_string_setting", test_wire_layout_of_string_setting},
        {"xresources_replace_managed_lines", test_xresources_replace_managed_lines},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
