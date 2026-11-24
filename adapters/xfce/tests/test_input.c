#include <glib.h>
#include <string.h>

#include "check.h"
#include "fake_source.h"
#include "input_resolve.h"
#include "input_text.h"
#include "system_keymap.h"

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

static void test_int_from_text(void)
{
    CHECK_INT_EQ(24, input_int_from_text("24").value);
    CHECK_INT_EQ(-90, input_int_from_text("-90.5").value);
    CHECK(!input_int_from_text("1e12").is_set);
    CHECK(!input_int_from_text("-1e20").is_set);
    CHECK(!input_int_from_text(NULL).is_set);
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

static void test_system_keymap_vconsole(void)
{
    system_keymap keymap = { 0 };
    system_keymap_read_vconsole(&keymap, "# XKBMODEL=pc105\nKEYMAP=dvorak\nXKBLAYOUT=us\n"
                                         " XKBVARIANT = \"dvorak\"\nXKBOPTIONS='compose:ralt'\n");

    CHECK(g_strcmp0(keymap.fields[SYSTEM_KEYMAP_LAYOUT], "us") == 0);
    CHECK(g_strcmp0(keymap.fields[SYSTEM_KEYMAP_VARIANT], "dvorak") == 0);
    CHECK(keymap.fields[SYSTEM_KEYMAP_MODEL] == NULL);
    CHECK(g_strcmp0(keymap.fields[SYSTEM_KEYMAP_OPTIONS], "compose:ralt") == 0);
    system_keymap_clear(&keymap);
}

static const char xorg_keyboard[] =
    "# keyboard\n"
    "Section \"InputClass\"\n"
    "    Identifier \"system-keyboard\"\n"
    "    MatchIsKeyboard \"on\"\n"
    "    Option \"XkbLayout\" \"us\" # main\n"
    "    Option \"Xkb_Variant\" \"alt-intl\"\n"
    "    Option \"XkbOptions\" \"terminate:ctrl_alt_bksp\"\n"
    "EndSection\n"
    "Section \"InputClass\"\n"
    "    MatchIsKeyboard \"on\"\n"
    "    MatchProduct \"Logitech\"\n"
    "    Option \"XkbLayout\" \"de\"\n"
    "EndSection\n"
    "Section \"InputDevice\"\n"
    "    Option \"XkbModel\" \"pc105\"\n"
    "EndSection\n";

static void test_system_keymap_xorg(void)
{
    system_keymap keymap = { 0 };
    system_keymap_read_xorg(&keymap, xorg_keyboard);

    CHECK(g_strcmp0(keymap.fields[SYSTEM_KEYMAP_LAYOUT], "us") == 0);
    CHECK(g_strcmp0(keymap.fields[SYSTEM_KEYMAP_VARIANT], "alt-intl") == 0);
    CHECK(keymap.fields[SYSTEM_KEYMAP_MODEL] == NULL);
    CHECK(g_strcmp0(keymap.fields[SYSTEM_KEYMAP_OPTIONS], "terminate:ctrl_alt_bksp") == 0);
    system_keymap_read_xorg(&keymap, "Section \"InputClass\"\nOption \"XkbLayout\" \"fr\"\n"
                                     "EndSection\n");
    CHECK(g_strcmp0(keymap.fields[SYSTEM_KEYMAP_LAYOUT], "fr") == 0);
    CHECK(g_strcmp0(keymap.fields[SYSTEM_KEYMAP_VARIANT], "alt-intl") == 0);
    system_keymap_clear(&keymap);
}

static void test_system_keymap_xorg_matches(void)
{
    system_keymap keymap = { 0 };
    system_keymap_read_xorg(&keymap, "Section \"InputClass\"\n"
                                     "    MatchIsKeyboard \"on\"\n"
                                     "    MatchDevicePath \"/dev/input/event*\"\n"
                                     "    MatchDriver \"evdev\"\n"
                                     "    MatchIsPointer \"off\"\n"
                                     "    Option \"XkbLayout\" \"fr\"\n"
                                     "EndSection\n"
                                     "Section \"InputClass\"\n"
                                     "    MatchIsPointer \"on\"\n"
                                     "    Option \"XkbLayout\" \"de\"\n"
                                     "EndSection\n"
                                     "Section \"InputClass\"\n"
                                     "    MatchIsKeyboard \"off\"\n"
                                     "    Option \"XkbLayout\" \"it\"\n"
                                     "EndSection\n");

    CHECK(g_strcmp0(keymap.fields[SYSTEM_KEYMAP_LAYOUT], "fr") == 0);
    system_keymap_clear(&keymap);
}

static system_keymap choose_keymap(const char *vconsole, const char *debian, const char *xorg,
                                   char **environment)
{
    system_keymap sources[3] = { 0 };
    system_keymap_read_vconsole(&sources[0], vconsole);
    system_keymap_read_vconsole(&sources[1], debian);
    system_keymap_read_xorg(&sources[2], xorg);
    system_keymap chosen = system_keymap_choose(sources, G_N_ELEMENTS(sources), environment);
    for (size_t i = 0; i < G_N_ELEMENTS(sources); i++) {
        for (int f = 0; f < SYSTEM_KEYMAP_FIELDS; f++) {
            CHECK(sources[i].fields[f] == NULL);
        }
    }
    return chosen;
}

static void test_system_keymap_choose(void)
{
    char *clean[] = { "HOME=/home/user", NULL };
    system_keymap chosen = choose_keymap("XKBOPTIONS=compose:ralt\n", "", xorg_keyboard, clean);
    CHECK(g_strcmp0(chosen.fields[SYSTEM_KEYMAP_LAYOUT], "us") == 0);
    CHECK(g_strcmp0(chosen.fields[SYSTEM_KEYMAP_VARIANT], "alt-intl") == 0);
    CHECK(g_strcmp0(chosen.fields[SYSTEM_KEYMAP_OPTIONS], "compose:ralt") == 0);
    system_keymap_clear(&chosen);

    chosen = choose_keymap("XKBLAYOUT=de\n", "XKBLAYOUT=br\n", xorg_keyboard, clean);
    CHECK(g_strcmp0(chosen.fields[SYSTEM_KEYMAP_LAYOUT], "de") == 0);
    CHECK(chosen.fields[SYSTEM_KEYMAP_VARIANT] == NULL);
    CHECK(g_strcmp0(chosen.fields[SYSTEM_KEYMAP_OPTIONS], "terminate:ctrl_alt_bksp") == 0);
    system_keymap_clear(&chosen);

    chosen = choose_keymap("KEYMAP=br-abnt2\n", "XKBLAYOUT=br\nXKBMODEL=abnt2\n", "", clean);
    CHECK(g_strcmp0(chosen.fields[SYSTEM_KEYMAP_LAYOUT], "br") == 0);
    CHECK(g_strcmp0(chosen.fields[SYSTEM_KEYMAP_MODEL], "abnt2") == 0);
    system_keymap_clear(&chosen);
}

static void test_system_keymap_environment(void)
{
    char *options[] = { "XKB_DEFAULT_OPTIONS=caps:escape", "XKB_DEFAULT_VARIANT=intl", NULL };
    char *layout[] = { "XKB_DEFAULT_LAYOUT=fr", "XKB_DEFAULT_MODEL=", NULL };
    const char *vconsole = "XKBLAYOUT=de\nXKBVARIANT=nodeadkeys\nXKBMODEL=pc105\n"
                           "XKBOPTIONS=compose:ralt\n";

    system_keymap chosen = choose_keymap(vconsole, "", "", options);
    CHECK(g_strcmp0(chosen.fields[SYSTEM_KEYMAP_LAYOUT], "de") == 0);
    CHECK(g_strcmp0(chosen.fields[SYSTEM_KEYMAP_VARIANT], "nodeadkeys") == 0);
    CHECK(chosen.fields[SYSTEM_KEYMAP_OPTIONS] == NULL);
    system_keymap_clear(&chosen);

    chosen = choose_keymap(vconsole, "", "", layout);
    CHECK(chosen.fields[SYSTEM_KEYMAP_LAYOUT] == NULL);
    CHECK(chosen.fields[SYSTEM_KEYMAP_VARIANT] == NULL);
    CHECK(g_strcmp0(chosen.fields[SYSTEM_KEYMAP_MODEL], "pc105") == 0);
    CHECK(g_strcmp0(chosen.fields[SYSTEM_KEYMAP_OPTIONS], "compose:ralt") == 0);
    system_keymap_clear(&chosen);
}

static void test_keyboard_system_keymap(void)
{
    static const fake_entry defaults[] = { { "XkbLayout", "fr" }, { NULL, NULL } };
    static const fake_entry custom[] = {
        { "XkbDisable", "false" }, { "XkbLayout", "fr" }, { NULL, NULL },
    };
    system_keymap system = { 0 };
    system_keymap_read_vconsole(&system, "XKBLAYOUT=us\nXKBVARIANT=dvorak\nXKBOPTIONS=caps:none\n");
    keyboard_sources sources = {
        .layout = fake_source(defaults),
        .keyboards = fake_source(no_entries),
        .system = &system,
    };
    keyboard_prefs prefs = input_keyboard_prefs_from(&sources);

    CHECK(strcmp(prefs.layout, "us") == 0);
    CHECK(strcmp(prefs.variant, "dvorak") == 0);
    CHECK(strcmp(prefs.options, "caps:none") == 0);
    sources.layout = fake_source(custom);
    prefs = input_keyboard_prefs_from(&sources);
    CHECK(strcmp(prefs.layout, "fr") == 0);
    CHECK(strcmp(prefs.variant, "") == 0);
    CHECK(strcmp(prefs.options, "") == 0);
    system_keymap_clear(&system);
}

static const fake_entry custom_layout[] = {
    { "XkbDisable", "false" }, { "XkbLayout", "us,ru" }, { "XkbVariant", ",phonetic" },
    { "XkbModel", "pc105" }, { "XkbOptions/Group", "grp:alt_shift_toggle" },
    { "XkbOptions/Compose", "compose:ralt" }, { NULL, NULL },
};

static const fake_entry custom_keyboards[] = {
    { "KeyRepeat/Delay", "300" }, { "KeyRepeat/Rate", "40" }, { "Numlock", "true" },
    { NULL, NULL },
};

static void test_keyboard_custom_layout(void)
{
    keyboard_sources sources = {
        .layout = fake_source(custom_layout),
        .keyboards = fake_source(custom_keyboards),
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
        { "int_from_text", test_int_from_text },
        { "keyboard_system_defaults", test_keyboard_system_defaults },
        { "system_keymap_vconsole", test_system_keymap_vconsole },
        { "system_keymap_xorg", test_system_keymap_xorg },
        { "system_keymap_xorg_matches", test_system_keymap_xorg_matches },
        { "system_keymap_choose", test_system_keymap_choose },
        { "system_keymap_environment", test_system_keymap_environment },
        { "keyboard_system_keymap", test_keyboard_system_keymap },
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
