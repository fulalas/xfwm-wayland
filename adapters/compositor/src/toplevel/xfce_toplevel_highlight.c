#include "util/listener.h"
#include "input/seat_resource.h"
#include "xfce_toplevel_private.h"

static void take_highlight(xfce_toplevels *toplevels, struct wl_client *client);
static void handle_owner_destroy(struct wl_listener *listener, void *data);

void xfce_handle_highlight(struct wl_client *client, struct wl_resource *resource,
                           struct wl_resource *seat)
{
    xfce_handle *handle = wl_resource_get_user_data(resource);
    xfce_toplevels *toplevels = handle->toplevels;
    wm_window *window = handle->entry != NULL ? view_window(handle->entry->view) : NULL;
    if (window == NULL || !seat_resource_is_ours(toplevels->server, seat)) {
        return;
    }
    take_highlight(toplevels, client);
    wm_request_highlight(toplevels->server->desktop, window);
}

void xfce_handle_unhighlight(struct wl_client *client, struct wl_resource *resource)
{
    xfce_handle *handle = wl_resource_get_user_data(resource);
    xfce_toplevels *toplevels = handle->toplevels;
    if (toplevels->highlight_owner != client) {
        return;
    }
    xfce_toplevels_release_highlight(toplevels);
    wm_request_unhighlight(toplevels->server->desktop);
}

void xfce_toplevels_release_highlight(xfce_toplevels *toplevels)
{
    listener_disconnect(&toplevels->owner_destroy);
    toplevels->highlight_owner = NULL;
}

static void take_highlight(xfce_toplevels *toplevels, struct wl_client *client)
{
    if (toplevels->highlight_owner == client) {
        return;
    }
    xfce_toplevels_release_highlight(toplevels);
    toplevels->highlight_owner = client;
    toplevels->owner_destroy.notify = handle_owner_destroy;
    wl_client_add_destroy_listener(client, &toplevels->owner_destroy);
}

static void handle_owner_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    xfce_toplevels *toplevels = wl_container_of(listener, toplevels, owner_destroy);
    xfce_toplevels_release_highlight(toplevels);
    wm_request_unhighlight(toplevels->server->desktop);
}
