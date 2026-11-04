#include "pointer_choices.h"

#include <glib.h>
#include <string.h>

#include "input_text.h"

enum {
    NO_FLAG = -1,
    FLAGS_MAX = 8,
    WACOM_CLOCKWISE = 1,
    WACOM_COUNTER_CLOCKWISE = 2,
    WACOM_HALF = 3,
    QUARTER_TURN = 90,
    HALF_TURN = 180,
    THREE_QUARTER_TURN = 270,
    HOMOGENEOUS_VALUES = 9,
};

typedef struct flag_list {
    int count;
    int first_set;
} flag_list;

static const pointer_accel_profile accel_profiles[] = {
    POINTER_ACCEL_PROFILE_ADAPTIVE,
    POINTER_ACCEL_PROFILE_FLAT,
};

static const pointer_click_method click_methods[] = {
    POINTER_CLICK_METHOD_BUTTON_AREAS,
    POINTER_CLICK_METHOD_CLICKFINGER,
};

static const pointer_scroll_method scroll_methods[] = {
    POINTER_SCROLL_METHOD_TWO_FINGER,
    POINTER_SCROLL_METHOD_EDGE,
    POINTER_SCROLL_METHOD_ON_BUTTON_DOWN,
};

static flag_list read_flags(const char *flags);
static bool all_numbers(gchar **parts);

pointer_accel_profile pointer_accel_profile_from_text(const char *flags)
{
    flag_list list = read_flags(flags);

    if (list.first_set == NO_FLAG || list.first_set >= (int) G_N_ELEMENTS(accel_profiles)) {
        return POINTER_ACCEL_PROFILE_DEFAULT;
    }
    return accel_profiles[list.first_set];
}

pointer_click_method pointer_click_method_from_text(const char *flags)
{
    flag_list list = read_flags(flags);

    if (list.count == 0) {
        return POINTER_CLICK_METHOD_DEFAULT;
    }
    if (list.first_set == NO_FLAG || list.first_set >= (int) G_N_ELEMENTS(click_methods)) {
        return POINTER_CLICK_METHOD_NONE;
    }
    return click_methods[list.first_set];
}

pointer_scroll_method pointer_scroll_method_from_text(const char *flags)
{
    flag_list list = read_flags(flags);

    if (list.count == 0) {
        return POINTER_SCROLL_METHOD_DEFAULT;
    }
    if (list.first_set == NO_FLAG || list.first_set >= (int) G_N_ELEMENTS(scroll_methods)) {
        return POINTER_SCROLL_METHOD_NONE;
    }
    return scroll_methods[list.first_set];
}

int pointer_rotation_from_wacom(int wacom_rotation)
{
    switch (wacom_rotation) {
    case WACOM_CLOCKWISE:
        return QUARTER_TURN;
    case WACOM_COUNTER_CLOCKWISE:
        return THREE_QUARTER_TURN;
    case WACOM_HALF:
        return HALF_TURN;
    default:
        return 0;
    }
}

unsigned pointer_reflection_from_text(const char *axes)
{
    unsigned reflection = 0;

    if (!axes) {
        return 0;
    }
    if (strchr(axes, 'X') || strchr(axes, 'x')) {
        reflection |= INPUT_REFLECT_X;
    }
    if (strchr(axes, 'Y') || strchr(axes, 'y')) {
        reflection |= INPUT_REFLECT_Y;
    }
    return reflection;
}

calibration_prefs pointer_calibration_from_text(const char *values)
{
    calibration_prefs calibration = { .is_set = false };
    gchar **parts = values ? g_strsplit(values, ",", -1) : NULL;
    guint count = parts ? g_strv_length(parts) : 0;

    if ((count == CALIBRATION_VALUES || count == HOMOGENEOUS_VALUES) && all_numbers(parts)) {
        calibration.is_set = true;
        for (int i = 0; i < CALIBRATION_VALUES; i++) {
            calibration.matrix[i] = (float) input_double_from_text(g_strstrip(parts[i])).value;
        }
    }
    g_strfreev(parts);
    return calibration;
}

static flag_list read_flags(const char *flags)
{
    flag_list list = { .count = 0, .first_set = NO_FLAG };
    gchar **parts = flags ? g_strsplit(flags, ",", FLAGS_MAX) : NULL;

    for (int i = 0; parts && parts[i]; i++) {
        optional_bool flag = input_bool_from_text(g_strstrip(parts[i]));

        if (!flag.is_set) {
            list.count = 0;
            list.first_set = NO_FLAG;
            break;
        }
        list.count++;
        if (flag.value && list.first_set == NO_FLAG) {
            list.first_set = i;
        }
    }
    g_strfreev(parts);
    return list;
}

static bool all_numbers(gchar **parts)
{
    for (int i = 0; parts[i]; i++) {
        if (!input_double_from_text(g_strstrip(parts[i])).is_set) {
            return false;
        }
    }
    return true;
}
