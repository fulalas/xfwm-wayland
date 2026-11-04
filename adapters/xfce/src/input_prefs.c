#include "input_prefs.h"

#include <stdlib.h>

#include "input_resolve.h"
#include "input_text.h"
#include "xfconf_source.h"

static const char default_prefix[] = "/Default/";

static const char *lookup_environment(void *self, const char *key);

keyboard_prefs input_prefs_keyboard(const channels *channels)
{
    xfconf_source *layout =
        xfconf_source_create(channels_get(channels, CHANNEL_KEYBOARD_LAYOUT), default_prefix);
    xfconf_source *keyboards =
        xfconf_source_create(channels_get(channels, CHANNEL_KEYBOARDS), default_prefix);
    keyboard_sources sources = {
        .layout = xfconf_source_view(layout),
        .keyboards = xfconf_source_view(keyboards),
    };
    keyboard_prefs prefs = input_keyboard_prefs_from(&sources);

    xfconf_source_destroy(layout);
    xfconf_source_destroy(keyboards);
    return prefs;
}

pointer_prefs input_prefs_pointer(const channels *channels, const char *device_name)
{
    char *prefix = input_device_property_prefix(device_name ? device_name : "");
    xfconf_source *device = xfconf_source_create(channels_get(channels, CHANNEL_POINTERS), prefix);
    setting_source view = xfconf_source_view(device);
    pointer_prefs prefs = input_pointer_prefs_from(&view);

    xfconf_source_destroy(device);
    g_free(prefix);
    return prefs;
}

cursor_prefs input_prefs_cursor(const channels *channels)
{
    xfconf_source *xsettings = xfconf_source_create(channels_get(channels, CHANNEL_XSETTINGS), "/");
    cursor_sources sources = {
        .xsettings = xfconf_source_view(xsettings),
        .environment = { .self = NULL, .lookup = lookup_environment },
    };
    cursor_prefs prefs = input_cursor_prefs_from(&sources);

    xfconf_source_destroy(xsettings);
    return prefs;
}

static const char *lookup_environment(void *self, const char *key)
{
    (void) self;
    return getenv(key);
}
