#ifndef COMPOSITOR_EXT_WORKSPACE_RESOURCE_H
#define COMPOSITOR_EXT_WORKSPACE_RESOURCE_H

#include <wayland-server-core.h>

struct wlr_ext_workspace_handle_v1;

struct wlr_ext_workspace_handle_v1 *ext_workspace_from_resource(struct wl_resource *resource);
struct wl_resource *ext_workspace_resource_of_client(
    struct wl_client *client, const struct wlr_ext_workspace_handle_v1 *workspace);

#endif
