#ifndef XFCE_INPUT_PREFS_H
#define XFCE_INPUT_PREFS_H

#include "channels.h"
#include "compositor/services.h"

keyboard_prefs input_prefs_keyboard(const channels *channels);
pointer_prefs input_prefs_pointer(const channels *channels, const char *device_name);
cursor_prefs input_prefs_cursor(const channels *channels);

#endif
