#ifndef COMPOSITOR_RESOURCE_H
#define COMPOSITOR_RESOURCE_H

#include <wayland-server-core.h>

struct wl_resource *resource_bind(struct wl_client *client, const struct wl_interface *interface,
                                  uint32_t version, uint32_t id, const void *implementation,
                                  void *data, wl_resource_destroy_func_t destroy);
void resource_handle_destroy(struct wl_client *client, struct wl_resource *resource);
void resource_unlink(struct wl_resource *resource);
void resource_detach(struct wl_resource *resource);

#endif
