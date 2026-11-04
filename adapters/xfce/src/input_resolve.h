#ifndef XFCE_INPUT_RESOLVE_H
#define XFCE_INPUT_RESOLVE_H

#include "compositor/services.h"
#include "setting_source.h"

typedef struct keyboard_sources {
    setting_source layout;
    setting_source keyboards;
} keyboard_sources;

typedef struct cursor_sources {
    setting_source xsettings;
    setting_source environment;
} cursor_sources;

keyboard_prefs input_keyboard_prefs_from(const keyboard_sources *sources);
pointer_prefs input_pointer_prefs_from(const setting_source *device);
cursor_prefs input_cursor_prefs_from(const cursor_sources *sources);

#endif
