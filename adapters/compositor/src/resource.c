#include "resource.h"

void resource_handle_destroy(struct wl_client *client, struct wl_resource *resource)
{
    (void) client;
    wl_resource_destroy(resource);
}
