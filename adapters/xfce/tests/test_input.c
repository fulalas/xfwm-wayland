#include <glib.h>
#include <string.h>

#include "check.h"
#include "fake_source.h"
#include "input_resolve.h"
#include "input_text.h"

static const fake_entry no_entries[] = { { NULL, NULL } };

static void test_device_prefix_mangling(void)
{
    char *prefix = input_device_property_prefix("SynPS/2 Synaptics TouchPad");

    CHECK(strcmp(prefix, "/SynPS2_Synaptics_TouchPad/") == 0);
    g_free(prefix);
    prefix = input_device_property_prefix("Logitech-USB_Mouse (2.4G)");
    CHECK(strcmp(prefix, "/Logitech-USB_Mouse_24G/") == 0);
    g_free(prefix);
}

static void test_join_options(void)
{
    const char *both[] = { "grp:alt_shift_toggle", "compose:ralt", NULL };
    const char *only_compose[] = { "", "compose:menu", NULL };
    const char *none[] = { "", "", NULL };
    char joined[PREFS_TEXT_MAX];

    input_join_options(both, joined, sizeof(joined));
    CHECK(strcmp(joined, "grp:alt_shift_toggle,compose:ralt") == 0);
    input_join_options(only_compose, joined, sizeof(joined));
    CHECK(strcmp(joined, "compose:menu") == 0);
    input_join_options(none, joined, sizeof(joined));
    CHECK(strcmp(joined, "") == 0);
}

static void test_bool_from_text(void)
{
    CHECK(input_bool_from_text("true").is_set && input_bool_from_text("true").value);
    CHECK(input_bool_from_text("FALSE").is_set && !input_bool_from_text("FALSE").value);
    CHECK(input_bool_from_text("1").value);
    CHECK(input_bool_from_text("0").is_set && !input_bool_from_text("0").value);
    CHECK(!input_bool_from_text(NULL).is_set);
    CHECK(!input_bool_from_text("maybe").is_set);
}

static void test_double_from_text(void)
{
    CHECK_DOUBLE_NEAR(-0.25, input_double_from_text("-0.25").value, 1e-9);
    CHECK(input_double_from_text("3").is_set);
    CHECK(!input_double_from_text("").is_set);
    CHECK(!input_double_from_text("x1").is_set);
}

static void test_keyboard_system_defaults(void)
{
    static const fake_entry layout[] = {
        { "XkbDisable", "true" }, { "XkbLayout", "fr" }, { NULL, NULL },
    };
    keyboard_sources sources = {
        .layout = fake_source(layout),
        .keyboards = fake_source(no_entries),
    };
    keyboard_prefs prefs = input_keyboard_prefs_from(&sources);

    CHECK(strcmp(prefs.layout, "") == 0);
    CHECK_INT_EQ(500, prefs.repeat_delay_ms);
    CHECK_INT_EQ(20, prefs.repeat_rate);
    CHECK(!prefs.numlock);
}

static void test_keyboard_custom_layout(void)
{
    static const fake_entry layout[] = {
        { "XkbDisable", "false" }, { "XkbLayout", "us,ru" }, { "XkbVariant", ",phonetic" },
        { "XkbModel", "pc105" }, { "XkbOptions/Group", "grp:alt_shift_toggle" },
        { "XkbOptions/Compose", "compose:ralt" }, { NULL, NULL },
    };
    static const fake_entry keyboards[] = {
        { "KeyRepeat/Delay", "300" }, { "KeyRepeat/Rate", "40" }, { "Numlock", "true" },
        { NULL, NULL },
    };
    keyboard_sources sources = {
        .layout = fake_source(layout),
        .keyboards = fake_source(keyboards),
    };
    keyboard_prefs prefs = input_keyboard_prefs_from(&sources);

    CHECK(strcmp(prefs.layout, "us,ru") == 0);
    CHECK(strcmp(prefs.variant, ",phonetic") == 0);
    CHECK(strcmp(prefs.model, "pc105") == 0);
    CHECK(strcmp(prefs.options, "grp:alt_shift_toggle,compose:ralt") == 0);
    CHECK_INT_EQ(300, prefs.repeat_delay_ms);
    CHECK_INT_EQ(40, prefs.repeat_rate);
    CHECK(prefs.numlock);
}

static void test_keyboard_repeat_disabled(void)
{
    static const fake_entry keyboards[] = { { "KeyRepeat", "false" }, { NULL, NULL } };
    keyboard_sources sources = {
        .layout = fake_source(no_entries),
        .keyboards = fake_source(keyboards),
    };

    CHECK_INT_EQ(0, input_keyboard_prefs_from(&sources).repeat_rate);
}

static void test_pointer_libinput_properties(void)
{
    static const fake_entry device[] = {
        { "Properties/libinput_Tapping_Enabled", "1" },
        { "Properties/libinput_Natural_Scrolling_Enabled", "0" },
        { "Properties/libinput_Left_Handed_Enabled", "true" },
        { "Properties/libinput_Accel_Speed", "2.5" },
        { "Properties/libinput_Disable_While_Typing_Enabled", "1" },
        { "ReverseScrolling", "true" }, { NULL, NULL },
    };
    setting_source source = fake_source(device);
    pointer_prefs prefs = input_pointer_prefs_from(&source);

    CHECK(prefs.tap.is_set && prefs.tap.value);
    CHECK(prefs.natural_scroll.is_set && !prefs.natural_scroll.value);
    CHECK(prefs.left_handed.is_set && prefs.left_handed.value);
    CHECK(prefs.accel_speed.is_set);
    CHECK_DOUBLE_NEAR(1.0, prefs.accel_speed.value, 1e-9);
    CHECK(prefs.disable_while_typing.is_set && prefs.disable_while_typing.value);
    CHECK(!prefs.middle_emulation.is_set);
}

static void test_pointer_legacy_fallbacks(void)
{
    static const fake_entry device[] = {
        { "ReverseScrolling", "true" }, { "RightHanded", "false" }, { "Acceleration", "2.5" },
        { NULL, NULL },
    };
    setting_source source = fake_source(device);
    pointer_prefs prefs = input_pointer_prefs_from(&source);

    CHECK(prefs.natural_scroll.is_set && prefs.natural_scroll.value);
    CHECK(prefs.left_handed.is_set && prefs.left_handed.value);
    CHECK_DOUBLE_NEAR(-0.5, prefs.accel_speed.value, 1e-9);
}

static void test_pointer_unset(void)
{
    setting_source source = fake_source(no_entries);
    pointer_prefs prefs = input_pointer_prefs_from(&source);

    CHECK(!prefs.tap.is_set);
    CHECK(!prefs.natural_scroll.is_set);
    CHECK(!prefs.left_handed.is_set);
    CHECK(!prefs.accel_speed.is_set);
}

static void test_cursor_from_xsettings(void)
{
    static const fake_entry xsettings[] = {
        { "Gtk/CursorThemeName", "Adwaita" }, { "Gtk/CursorThemeSize", "32" }, { NULL, NULL },
    };
    static const fake_entry environment[] = {
        { "XCURSOR_THEME", "other" }, { "XCURSOR_SIZE", "48" }, { NULL, NULL },
    };
    cursor_sources sources = { fake_source(xsettings), fake_source(environment) };
    cursor_prefs prefs = input_cursor_prefs_from(&sources);

    CHECK(strcmp(prefs.theme, "Adwaita") == 0);
    CHECK_INT_EQ(32, prefs.size);
}

static void test_cursor_fallbacks(void)
{
    static const fake_entry environment[] = {
        { "XCURSOR_THEME", "breeze" }, { "XCURSOR_SIZE", "48" }, { NULL, NULL },
    };
    cursor_sources from_environment = { fake_source(no_entries), fake_source(environment) };
    cursor_sources from_nothing = { fake_source(no_entries), fake_source(no_entries) };
    cursor_prefs prefs = input_cursor_prefs_from(&from_environment);

    CHECK(strcmp(prefs.theme, "breeze") == 0);
    CHECK_INT_EQ(48, prefs.size);
    prefs = input_cursor_prefs_from(&from_nothing);
    CHECK(strcmp(prefs.theme, "default") == 0);
    CHECK_INT_EQ(24, prefs.size);
}

int main(void)
{
    static const check_case cases[] = {
        { "device_prefix_mangling", test_device_prefix_mangling },
        { "join_options", test_join_options },
        { "bool_from_text", test_bool_from_text },
        { "double_from_text", test_double_from_text },
        { "keyboard_system_defaults", test_keyboard_system_defaults },
        { "keyboard_custom_layout", test_keyboard_custom_layout },
        { "keyboard_repeat_disabled", test_keyboard_repeat_disabled },
        { "pointer_libinput_properties", test_pointer_libinput_properties },
        { "pointer_legacy_fallbacks", test_pointer_legacy_fallbacks },
        { "pointer_unset", test_pointer_unset },
        { "cursor_from_xsettings", test_cursor_from_xsettings },
        { "cursor_fallbacks", test_cursor_fallbacks },
    };

    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
