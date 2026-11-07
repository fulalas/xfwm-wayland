#ifndef COMPOSITOR_TOPLEVEL_ICONS_H
#define COMPOSITOR_TOPLEVEL_ICONS_H

#include "server/server.h"

toplevel_icons *toplevel_icons_create(compositor *server);
void toplevel_icons_destroy(toplevel_icons *icons);

#endif
