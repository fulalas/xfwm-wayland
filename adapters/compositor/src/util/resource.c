#include "resource.h"

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
