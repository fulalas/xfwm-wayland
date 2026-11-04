#ifndef COMPOSITOR_XRESOURCES_H
#define COMPOSITOR_XRESOURCES_H

#include "xsettings_table.h"

char *xresources_merge(const char *existing, const xsettings_table *settings);

#endif
