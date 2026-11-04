#include "input_resolve.h"

#include <glib.h>

#include "input_text.h"

enum {
    DEFAULT_REPEAT_DELAY_MS = 500,
    DEFAULT_REPEAT_RATE = 20,
    DEFAULT_CURSOR_SIZE = 24,
    ACCELERATION_STEPS = 5,
};

static const char default_cursor_theme[] = "default";

static bool lookup_bool(const setting_source *source, const char *key, bool fallback);
static int lookup_int(const setting_source *source, const char *key, int fallback);
static void copy_text(const setting_source *source, const char *key, char *target);
static void copy_options(const setting_source *layout, char *target);
static optional_bool natural_scroll(const setting_source *device);
static optional_bool left_handed(const setting_source *device);
static optional_double accel_speed(const setting_source *device);
static optional_double legacy_accel_speed(const setting_source *device);
static optional_bool device_bool(const setting_source *device, const char *key);

keyboard_prefs input_keyboard_prefs_from(const keyboard_sources *sources)
{
    keyboard_prefs prefs = { .numlock = lookup_bool(&sources->keyboards, "Numlock", false) };

    prefs.repeat_delay_ms =
        lookup_int(&sources->keyboards, "KeyRepeat/Delay", DEFAULT_REPEAT_DELAY_MS);
    prefs.repeat_rate = lookup_int(&sources->keyboards, "KeyRepeat/Rate", DEFAULT_REPEAT_RATE);
    if (!lookup_bool(&sources->keyboards, "KeyRepeat", true)) {
        prefs.repeat_rate = 0;
    }
    if (lookup_bool(&sources->layout, "XkbDisable", true)) {
        return prefs;
    }
    copy_text(&sources->layout, "XkbLayout", prefs.layout);
    copy_text(&sources->layout, "XkbVariant", prefs.variant);
    copy_text(&sources->layout, "XkbModel", prefs.model);
    copy_options(&sources->layout, prefs.options);
    return prefs;
}

pointer_prefs input_pointer_prefs_from(const setting_source *device)
{
    return (pointer_prefs) {
        .tap = device_bool(device, "libinput_Tapping_Enabled"),
        .natural_scroll = natural_scroll(device),
        .left_handed = left_handed(device),
        .accel_speed = accel_speed(device),
        .disable_while_typing = device_bool(device, "libinput_Disable_While_Typing_Enabled"),
        .middle_emulation = device_bool(device, "libinput_Middle_Emulation_Enabled"),
    };
}

cursor_prefs input_cursor_prefs_from(const cursor_sources *sources)
{
    const char *theme = setting_source_lookup(&sources->xsettings, "Gtk/CursorThemeName");
    int size = lookup_int(&sources->xsettings, "Gtk/CursorThemeSize", 0);
    cursor_prefs prefs = { .size = size };

    if (!theme || *theme == '\0') {
        theme = setting_source_lookup(&sources->environment, "XCURSOR_THEME");
    }
    if (prefs.size <= 0) {
        prefs.size = lookup_int(&sources->environment, "XCURSOR_SIZE", 0);
    }
    if (prefs.size <= 0) {
        prefs.size = DEFAULT_CURSOR_SIZE;
    }
    g_strlcpy(prefs.theme, theme && *theme ? theme : default_cursor_theme, sizeof(prefs.theme));
    return prefs;
}

static bool lookup_bool(const setting_source *source, const char *key, bool fallback)
{
    optional_bool value = input_bool_from_text(setting_source_lookup(source, key));

    return value.is_set ? value.value : fallback;
}

static int lookup_int(const setting_source *source, const char *key, int fallback)
{
    optional_double value = input_double_from_text(setting_source_lookup(source, key));

    return value.is_set ? (int) value.value : fallback;
}

static void copy_text(const setting_source *source, const char *key, char *target)
{
    const char *text = setting_source_lookup(source, key);

    g_strlcpy(target, text ? text : "", PREFS_TEXT_MAX);
}

static void copy_options(const setting_source *layout, char *target)
{
    const char *group = setting_source_lookup(layout, "XkbOptions/Group");
    const char *compose = setting_source_lookup(layout, "XkbOptions/Compose");
    const char *parts[] = { group ? group : "", compose ? compose : "", NULL };

    input_join_options(parts, target, PREFS_TEXT_MAX);
}

static optional_bool natural_scroll(const setting_source *device)
{
    optional_bool value = device_bool(device, "libinput_Natural_Scrolling_Enabled");

    if (value.is_set) {
        return value;
    }
    return input_bool_from_text(setting_source_lookup(device, "ReverseScrolling"));
}

static optional_bool left_handed(const setting_source *device)
{
    optional_bool value = device_bool(device, "libinput_Left_Handed_Enabled");

    if (value.is_set) {
        return value;
    }
    value = input_bool_from_text(setting_source_lookup(device, "RightHanded"));
    value.value = !value.value;
    return value;
}

static optional_double accel_speed(const setting_source *device)
{
    const char *speed = setting_source_lookup(device, "Properties/libinput_Accel_Speed");
    optional_double value = input_double_from_text(speed);

    if (!value.is_set) {
        value = legacy_accel_speed(device);
    }
    value.value = CLAMP(value.value, -1.0, 1.0);
    return value;
}

static optional_double legacy_accel_speed(const setting_source *device)
{
    optional_double value = input_double_from_text(setting_source_lookup(device, "Acceleration"));

    if (value.is_set) {
        value.value = value.value / ACCELERATION_STEPS - 1.0;
    }
    return value;
}

static optional_bool device_bool(const setting_source *device, const char *key)
{
    char *property = g_strconcat("Properties/", key, NULL);
    optional_bool value = input_bool_from_text(setting_source_lookup(device, property));

    g_free(property);
    return value;
}
