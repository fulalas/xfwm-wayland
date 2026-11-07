#ifndef COMPOSITOR_SEAT_RESOURCE_H
#define COMPOSITOR_SEAT_RESOURCE_H

#include <stdbool.h>
#include <wayland-server-core.h>

#include "server/server.h"

bool seat_resource_is_ours(const compositor *server, struct wl_resource *seat);

#endif
