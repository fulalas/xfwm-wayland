#include "resource.h"

struct wl_resource *resource_bind(struct wl_client *client, const struct wl_interface *interface,
                                  uint32_t version, uint32_t id, const void *implementation,
                                  void *data, wl_resource_destroy_func_t destroy)
{
    struct wl_resource *resource = wl_resource_create(client, interface, (int) version, id);
    if (resource == NULL) {
        wl_client_post_no_memory(client);
        return NULL;
    }
    wl_resource_set_implementation(resource, implementation, data, destroy);
    return resource;
}

void resource_handle_destroy(struct wl_client *client, struct wl_resource *resource)
{
    (void) client;
    wl_resource_destroy(resource);
}

void resource_unlink(struct wl_resource *resource)
{
    wl_list_remove(wl_resource_get_link(resource));
}

void resource_detach(struct wl_resource *resource)
{
    wl_resource_set_user_data(resource, NULL);
    wl_list_remove(wl_resource_get_link(resource));
    wl_list_init(wl_resource_get_link(resource));
}
