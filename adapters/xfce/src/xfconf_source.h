#ifndef XFCE_XFCONF_SOURCE_H
#define XFCE_XFCONF_SOURCE_H

#include <xfconf/xfconf.h>

#include "setting_source.h"

typedef struct xfconf_source xfconf_source;

xfconf_source *xfconf_source_create(XfconfChannel *channel, const char *prefix);
void xfconf_source_destroy(xfconf_source *source);
setting_source xfconf_source_view(xfconf_source *source);

#endif
