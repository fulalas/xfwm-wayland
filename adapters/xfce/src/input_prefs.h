#ifndef XFCE_INPUT_PREFS_H
#define XFCE_INPUT_PREFS_H

#include "channels.h"
#include "compositor/services.h"
#include "system_keymap.h"

system_keymap input_prefs_system_keymap(char **environment);
keyboard_prefs input_prefs_keyboard(const channels *channels, const system_keymap *system);
pointer_prefs input_prefs_pointer(const channels *channels, const char *device_name);
cursor_prefs input_prefs_cursor(const channels *channels, char **environment);

#endif
