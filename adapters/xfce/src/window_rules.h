#ifndef XFCE_WINDOW_RULES_H
#define XFCE_WINDOW_RULES_H

#include <glib-object.h>

#include "xfce/environment.h"

extern const char window_rules_root[];

xfce_window_rules window_rules_from(GHashTable *properties);

#endif
