#ifndef COMPOSITOR_XFCE_TOPLEVEL_H
#define COMPOSITOR_XFCE_TOPLEVEL_H

#include "foreign_toplevel_private.h"

xfce_toplevels *xfce_toplevels_create(compositor *server);
void xfce_toplevels_destroy(xfce_toplevels *toplevels);
void xfce_toplevels_publish(toplevel_entry *entry);
void xfce_toplevels_detach(toplevel_entry *entry);

#endif
