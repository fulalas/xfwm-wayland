#ifndef COMPOSITOR_X11_ACCESS_H
#define COMPOSITOR_X11_ACCESS_H

#include <stdbool.h>
#include <xcb/xcb.h>

void x11_access_allow_root(xcb_connection_t *connection);
bool x11_access_allow_root_on(const char *display);

#endif
