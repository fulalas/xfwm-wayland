#ifndef XFCE_POINTER_CHOICES_H
#define XFCE_POINTER_CHOICES_H

#include "compositor/services.h"

pointer_accel_profile pointer_accel_profile_from_text(const char *flags);
pointer_click_method pointer_click_method_from_text(const char *flags);
pointer_scroll_method pointer_scroll_method_from_text(const char *flags);
int pointer_rotation_from_wacom(int wacom_rotation);
unsigned pointer_reflection_from_text(const char *axes);
calibration_prefs pointer_calibration_from_text(const char *values);

#endif
