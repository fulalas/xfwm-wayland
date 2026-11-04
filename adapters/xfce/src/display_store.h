#ifndef XFCE_DISPLAY_STORE_H
#define XFCE_DISPLAY_STORE_H

#include <xfconf/xfconf.h>

#include "compositor/displays.h"

display_plan display_store_load(XfconfChannel *channel, const display_set *connected);
void display_store_save(XfconfChannel *channel, const display_set *current);

#endif
