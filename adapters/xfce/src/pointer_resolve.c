#include <glib.h>

#include "input_resolve.h"
#include "input_text.h"
#include "pointer_choices.h"

enum {
    ACCELERATION_STEPS = 5,
};

static pointer_prefs basic_prefs(const setting_source *device);
static pointer_prefs with_methods(pointer_prefs prefs, const setting_source *device);
static absolute_prefs absolute_prefs_from(const setting_source *device);
static int rotation_degrees(const setting_source *device);
static optional_bool natural_scroll(const setting_source *device);
static optional_bool left_handed(const setting_source *device);
static optional_double accel_speed(const setting_source *device);
static optional_double legacy_accel_speed(const setting_source *device);
static optional_bool device_bool(const setting_source *device, const char *key);
static optional_int device_int(const setting_source *device, const char *key);
static const char *device_property(const setting_source *device, const char *key);

pointer_prefs input_pointer_prefs_from(const setting_source *device)
{
    pointer_prefs prefs = with_methods(basic_prefs(device), device);

    prefs.absolute = absolute_prefs_from(device);
    return prefs;
}

static pointer_prefs basic_prefs(const setting_source *device)
{
    return (pointer_prefs) {
        .tap = device_bool(device, "libinput_Tapping_Enabled"),
        .natural_scroll = natural_scroll(device),
        .left_handed = left_handed(device),
        .accel_speed = accel_speed(device),
        .disable_while_typing = device_bool(device, "libinput_Disable_While_Typing_Enabled"),
        .middle_emulation = device_bool(device, "libinput_Middle_Emulation_Enabled"),
        .enabled = device_bool(device, "Device_Enabled"),
    };
}

static pointer_prefs with_methods(pointer_prefs prefs, const setting_source *device)
{
    const char *profile = device_property(device, "libinput_Accel_Profile_Enabled");
    const char *click = device_property(device, "libinput_Click_Method_Enabled");
    const char *scroll = device_property(device, "libinput_Scroll_Method_Enabled");

    prefs.accel_profile = pointer_accel_profile_from_text(profile);
    prefs.click_method = pointer_click_method_from_text(click);
    prefs.scroll_method = pointer_scroll_method_from_text(scroll);
    prefs.scroll_button = device_int(device, "libinput_Button_Scrolling_Button");
    prefs.scroll_button_lock = device_bool(device, "libinput_Button_Scrolling_Button_Lock_Enabled");
    return prefs;
}

static absolute_prefs absolute_prefs_from(const setting_source *device)
{
    const char *output = setting_source_lookup(device, "AssignedMonitor");
    const char *calibration = device_property(device, "libinput_Calibration_Matrix");
    absolute_prefs prefs = {
        .rotation_degrees = rotation_degrees(device),
        .reflection = pointer_reflection_from_text(setting_source_lookup(device, "Reflection")),
        .calibration = pointer_calibration_from_text(calibration),
    };

    g_strlcpy(prefs.output_id, output ? output : "", sizeof(prefs.output_id));
    return prefs;
}

static int rotation_degrees(const setting_source *device)
{
    optional_double degrees = input_double_from_text(setting_source_lookup(device, "Rotation"));
    optional_int wacom = device_int(device, "Wacom_Rotation");

    if (degrees.is_set) {
        return (int) degrees.value;
    }
    return wacom.is_set ? pointer_rotation_from_wacom(wacom.value) : 0;
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
    optional_double value = input_double_from_text(device_property(device, "libinput_Accel_Speed"));

    if (!value.is_set) {
        value = legacy_accel_speed(device);
    }
    value.value = CLAMP(value.value, -1.0, 1.0);
    return value;
}

static optional_double legacy_accel_speed(const setting_source *device)
{
    optional_double value = input_double_from_text(setting_source_lookup(device, "Acceleration"));

    if (value.is_set && value.value < 0.0) {
        return (optional_double) { .is_set = false };
    }
    if (value.is_set) {
        value.value = value.value / ACCELERATION_STEPS - 1.0;
    }
    return value;
}

static optional_bool device_bool(const setting_source *device, const char *key)
{
    return input_bool_from_text(device_property(device, key));
}

static optional_int device_int(const setting_source *device, const char *key)
{
    optional_double value = input_double_from_text(device_property(device, key));

    return (optional_int) { .is_set = value.is_set, .value = (int) value.value };
}

static const char *device_property(const setting_source *device, const char *key)
{
    char *property = g_strconcat("Properties/", key, NULL);
    const char *text = setting_source_lookup(device, property);

    g_free(property);
    return text;
}
