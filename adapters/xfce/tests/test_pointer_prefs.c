#include <string.h>

#include "check.h"
#include "fake_source.h"
#include "input_resolve.h"
#include "pointer_choices.h"

static const fake_entry no_entries[] = { { NULL, NULL } };

static void test_accel_profile_flags(void)
{
    CHECK_INT_EQ(POINTER_ACCEL_PROFILE_ADAPTIVE, pointer_accel_profile_from_text("1,0,0"));
    CHECK_INT_EQ(POINTER_ACCEL_PROFILE_FLAT, pointer_accel_profile_from_text("0,1"));
    CHECK_INT_EQ(POINTER_ACCEL_PROFILE_DEFAULT, pointer_accel_profile_from_text("0,0,1"));
    CHECK_INT_EQ(POINTER_ACCEL_PROFILE_DEFAULT, pointer_accel_profile_from_text("0,0"));
    CHECK_INT_EQ(POINTER_ACCEL_PROFILE_DEFAULT, pointer_accel_profile_from_text(NULL));
}

static void test_click_method_flags(void)
{
    CHECK_INT_EQ(POINTER_CLICK_METHOD_BUTTON_AREAS, pointer_click_method_from_text("1,0"));
    CHECK_INT_EQ(POINTER_CLICK_METHOD_CLICKFINGER, pointer_click_method_from_text("0,1"));
    CHECK_INT_EQ(POINTER_CLICK_METHOD_NONE, pointer_click_method_from_text("0,0"));
    CHECK_INT_EQ(POINTER_CLICK_METHOD_DEFAULT, pointer_click_method_from_text(NULL));
    CHECK_INT_EQ(POINTER_CLICK_METHOD_DEFAULT, pointer_click_method_from_text("x,1"));
}

static void test_scroll_method_flags(void)
{
    CHECK_INT_EQ(POINTER_SCROLL_METHOD_TWO_FINGER, pointer_scroll_method_from_text("1,0,0"));
    CHECK_INT_EQ(POINTER_SCROLL_METHOD_EDGE, pointer_scroll_method_from_text("0,1,0"));
    CHECK_INT_EQ(POINTER_SCROLL_METHOD_ON_BUTTON_DOWN, pointer_scroll_method_from_text("0,0,1"));
    CHECK_INT_EQ(POINTER_SCROLL_METHOD_NONE, pointer_scroll_method_from_text("0,0,0"));
    CHECK_INT_EQ(POINTER_SCROLL_METHOD_EDGE, pointer_scroll_method_from_text("false,true,false"));
    CHECK_INT_EQ(POINTER_SCROLL_METHOD_DEFAULT, pointer_scroll_method_from_text(""));
}

static void test_wacom_rotation(void)
{
    CHECK_INT_EQ(0, pointer_rotation_from_wacom(0));
    CHECK_INT_EQ(90, pointer_rotation_from_wacom(1));
    CHECK_INT_EQ(270, pointer_rotation_from_wacom(2));
    CHECK_INT_EQ(180, pointer_rotation_from_wacom(3));
    CHECK_INT_EQ(0, pointer_rotation_from_wacom(7));
}

static void test_reflection_text(void)
{
    CHECK_INT_EQ(0, pointer_reflection_from_text("0"));
    CHECK_INT_EQ(INPUT_REFLECT_X, pointer_reflection_from_text("X"));
    CHECK_INT_EQ(INPUT_REFLECT_Y, pointer_reflection_from_text("Y"));
    CHECK_INT_EQ(INPUT_REFLECT_X | INPUT_REFLECT_Y, pointer_reflection_from_text("XY"));
    CHECK_INT_EQ(0, pointer_reflection_from_text(NULL));
}

static void test_calibration_text(void)
{
    calibration_prefs six = pointer_calibration_from_text("1.1,0,-0.05,0,1.2,-0.1");
    calibration_prefs nine = pointer_calibration_from_text("0,-1,1,1,0,0,0,0,1");

    CHECK(six.is_set);
    CHECK_DOUBLE_NEAR(1.1, six.matrix[0], 1e-6);
    CHECK_DOUBLE_NEAR(-0.1, six.matrix[5], 1e-6);
    CHECK(nine.is_set);
    CHECK_DOUBLE_NEAR(-1.0, nine.matrix[1], 1e-6);
    CHECK_DOUBLE_NEAR(1.0, nine.matrix[3], 1e-6);
    CHECK(!pointer_calibration_from_text("1,0,0").is_set);
    CHECK(!pointer_calibration_from_text("1,0,0,0,1,y").is_set);
    CHECK(!pointer_calibration_from_text(NULL).is_set);
}

static void test_dialog_properties(void)
{
    static const fake_entry device[] = {
        { "Properties/Device_Enabled", "0" },
        { "Properties/libinput_Accel_Profile_Enabled", "0,1,0" },
        { "Properties/libinput_Click_Method_Enabled", "0,1" },
        { "Properties/libinput_Scroll_Method_Enabled", "0,0,1" },
        { "Properties/libinput_Button_Scrolling_Button", "274" },
        { NULL, NULL },
    };
    setting_source source = fake_source(device);
    pointer_prefs prefs = input_pointer_prefs_from(&source);

    CHECK(prefs.enabled.is_set && !prefs.enabled.value);
    CHECK_INT_EQ(POINTER_ACCEL_PROFILE_FLAT, prefs.accel_profile);
    CHECK_INT_EQ(POINTER_CLICK_METHOD_CLICKFINGER, prefs.click_method);
    CHECK_INT_EQ(POINTER_SCROLL_METHOD_ON_BUTTON_DOWN, prefs.scroll_method);
    CHECK(prefs.scroll_button.is_set);
    CHECK_INT_EQ(274, prefs.scroll_button.value);
}

static void test_missing_properties_mean_device_default(void)
{
    setting_source source = fake_source(no_entries);
    pointer_prefs prefs = input_pointer_prefs_from(&source);

    CHECK(!prefs.enabled.is_set);
    CHECK_INT_EQ(POINTER_ACCEL_PROFILE_DEFAULT, prefs.accel_profile);
    CHECK_INT_EQ(POINTER_CLICK_METHOD_DEFAULT, prefs.click_method);
    CHECK_INT_EQ(POINTER_SCROLL_METHOD_DEFAULT, prefs.scroll_method);
    CHECK(!prefs.scroll_button.is_set);
    CHECK(!prefs.absolute.calibration.is_set);
    CHECK(strcmp(prefs.absolute.output_id, "") == 0);
    CHECK_INT_EQ(0, prefs.absolute.rotation_degrees);
}

static void test_negative_acceleration_means_default(void)
{
    static const fake_entry device[] = { { "Acceleration", "-1" }, { NULL, NULL } };
    setting_source source = fake_source(device);

    CHECK(!input_pointer_prefs_from(&source).accel_speed.is_set);
}

static void test_touchscreen_mapping(void)
{
    static const fake_entry device[] = {
        { "AssignedMonitor", "3f786850e387550fdab836ed7e6dc881de23001b" },
        { "Rotation", "270" },
        { "Reflection", "X" },
        { NULL, NULL },
    };
    setting_source source = fake_source(device);
    absolute_prefs prefs = input_pointer_prefs_from(&source).absolute;

    CHECK(strcmp(prefs.output_id, "3f786850e387550fdab836ed7e6dc881de23001b") == 0);
    CHECK_INT_EQ(270, prefs.rotation_degrees);
    CHECK_INT_EQ(INPUT_REFLECT_X, prefs.reflection);
}

static void test_tablet_rotation(void)
{
    static const fake_entry wacom[] = { { "Properties/Wacom_Rotation", "2" }, { NULL, NULL } };
    static const fake_entry both[] = {
        { "Rotation", "90" }, { "Properties/Wacom_Rotation", "3" }, { NULL, NULL },
    };
    setting_source tablet = fake_source(wacom);
    setting_source mixed = fake_source(both);

    CHECK_INT_EQ(270, input_pointer_prefs_from(&tablet).absolute.rotation_degrees);
    CHECK_INT_EQ(90, input_pointer_prefs_from(&mixed).absolute.rotation_degrees);
}

int main(void)
{
    static const check_case cases[] = {
        { "accel_profile_flags", test_accel_profile_flags },
        { "click_method_flags", test_click_method_flags },
        { "scroll_method_flags", test_scroll_method_flags },
        { "wacom_rotation", test_wacom_rotation },
        { "reflection_text", test_reflection_text },
        { "calibration_text", test_calibration_text },
        { "dialog_properties", test_dialog_properties },
        { "missing_properties_mean_device_default", test_missing_properties_mean_device_default },
        { "negative_acceleration_means_default", test_negative_acceleration_means_default },
        { "touchscreen_mapping", test_touchscreen_mapping },
        { "tablet_rotation", test_tablet_rotation },
    };

    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
