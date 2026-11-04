#include "accessibility.h"

#include "accessibility_resolve.h"
#include "xfconf_source.h"

static const char root_prefix[] = "/";

accessibility_prefs accessibility_prefs_load(const channels *channels)
{
    xfconf_source *source =
        xfconf_source_create(channels_get(channels, CHANNEL_ACCESSIBILITY), root_prefix);
    setting_source view = xfconf_source_view(source);
    accessibility_prefs prefs = accessibility_prefs_from(&view);

    xfconf_source_destroy(source);
    return prefs;
}
