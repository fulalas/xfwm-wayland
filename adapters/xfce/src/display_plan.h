#ifndef XFCE_DISPLAY_PLAN_H
#define XFCE_DISPLAY_PLAN_H

#include <glib.h>

#include "compositor/displays.h"

display_plan display_profile_plan(GHashTable *table, const display_set *connected);

#endif
