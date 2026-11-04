#ifndef XFCE_X11_SETTINGS_H
#define XFCE_X11_SETTINGS_H

#include <xfconf/xfconf.h>

#include "compositor/x11_settings.h"

void x11_settings_collect(XfconfChannel *channel, const x11_setting_sink *sink);

#endif
