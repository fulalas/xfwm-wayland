#ifndef XFCE_X11_SETTING_VALUES_H
#define XFCE_X11_SETTING_VALUES_H

#include <glib-object.h>
#include <stdbool.h>

#include "compositor/x11_settings.h"

bool x11_setting_from_property(const char *property, const GValue *value, x11_setting *setting);

#endif
