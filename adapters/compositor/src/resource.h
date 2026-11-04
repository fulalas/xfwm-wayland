#ifndef COMPOSITOR_RESOURCE_H
#define COMPOSITOR_RESOURCE_H

#include <wayland-server-core.h>

void resource_handle_destroy(struct wl_client *client, struct wl_resource *resource);

#endif
