#ifndef COMPOSITOR_SURFACE_WATCH_H
#define COMPOSITOR_SURFACE_WATCH_H

#include "seat.h"

typedef struct surface_watch surface_watch;

surface_watch *surface_watch_create(seat *seat);
void surface_watch_destroy(surface_watch *watch);

#endif
