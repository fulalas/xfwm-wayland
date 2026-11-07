#ifndef COMPOSITOR_UNMANAGED_H
#define COMPOSITOR_UNMANAGED_H

#include "server/server.h"

struct wlr_xwayland_surface;

typedef struct unmanaged unmanaged;

unmanaged *unmanaged_create(compositor *server, struct wlr_xwayland_surface *xsurface);
void unmanaged_destroy(unmanaged *unmanaged);
void unmanaged_move(unmanaged *unmanaged);
void unmanaged_committed(unmanaged *unmanaged);

#endif
