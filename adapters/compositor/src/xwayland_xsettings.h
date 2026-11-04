#ifndef COMPOSITOR_XWAYLAND_XSETTINGS_H
#define COMPOSITOR_XWAYLAND_XSETTINGS_H

#include <stdbool.h>
#include <xcb/xcb.h>

#include "server.h"
#include "x11_link.h"

typedef struct xwayland_xsettings xwayland_xsettings;

xwayland_xsettings *xwayland_xsettings_create(compositor *server, const x11_link *link);
void xwayland_xsettings_destroy(xwayland_xsettings *xsettings);
void xwayland_xsettings_link_ready(xwayland_xsettings *xsettings);
void xwayland_xsettings_reload(xwayland_xsettings *xsettings);
bool xwayland_xsettings_handle_event(xwayland_xsettings *xsettings, xcb_generic_event_t *event);

#endif
