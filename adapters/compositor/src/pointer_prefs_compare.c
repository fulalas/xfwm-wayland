#include <string.h>

#include "pointer_prefs_compare.h"

static bool same_switches(const pointer_prefs *a, const pointer_prefs *b);
static bool same_methods(const pointer_prefs *a, const pointer_prefs *b);
static bool same_absolute(const absolute_prefs *a, const absolute_prefs *b);
static bool same_calibration(const calibration_prefs *a, const calibration_prefs *b);
static bool same_choice(optional_bool a, optional_bool b);
static bool same_number(optional_double a, optional_double b);
static bool same_integer(optional_int a, optional_int b);

bool pointer_prefs_equal(const pointer_prefs *a, const pointer_prefs *b)
{
    return same_switches(a, b) && same_methods(a, b) && same_absolute(&a->absolute, &b->absolute);
}

static bool same_switches(const pointer_prefs *a, const pointer_prefs *b)
{
    return same_choice(a->tap, b->tap) && same_choice(a->natural_scroll, b->natural_scroll) &&
           same_choice(a->left_handed, b->left_handed) &&
           same_choice(a->disable_while_typing, b->disable_while_typing) &&
           same_choice(a->middle_emulation, b->middle_emulation) &&
           same_choice(a->enabled, b->enabled) &&
           same_choice(a->scroll_button_lock, b->scroll_button_lock);
}

static bool same_methods(const pointer_prefs *a, const pointer_prefs *b)
{
    return same_number(a->accel_speed, b->accel_speed) && a->accel_profile == b->accel_profile &&
           a->click_method == b->click_method && a->scroll_method == b->scroll_method &&
           same_integer(a->scroll_button, b->scroll_button);
}

static bool same_absolute(const absolute_prefs *a, const absolute_prefs *b)
{
    return strcmp(a->output_id, b->output_id) == 0 &&
           a->rotation_degrees == b->rotation_degrees && a->reflection == b->reflection &&
           same_calibration(&a->calibration, &b->calibration);
}

static bool same_calibration(const calibration_prefs *a, const calibration_prefs *b)
{
    if (a->is_set != b->is_set) {
        return false;
    }
    for (int i = 0; a->is_set && i < CALIBRATION_VALUES; i++) {
        if (a->matrix[i] != b->matrix[i]) {
            return false;
        }
    }
    return true;
}

static bool same_choice(optional_bool a, optional_bool b)
{
    return a.is_set == b.is_set && (!a.is_set || a.value == b.value);
}

static bool same_number(optional_double a, optional_double b)
{
    return a.is_set == b.is_set && (!a.is_set || a.value == b.value);
}

static bool same_integer(optional_int a, optional_int b)
{
    return a.is_set == b.is_set && (!a.is_set || a.value == b.value);
}
