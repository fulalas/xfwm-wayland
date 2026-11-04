#include "client_surfaces.h"

#include <string.h>
#include <wayland-server-core.h>
#include <wlr/types/wlr_compositor.h>

typedef struct visit_request {
    surface_visitor visit;
    void *data;
} visit_request;

static enum wl_iterator_result visit_resource(struct wl_resource *resource, void *data);

void client_surfaces_for_each(struct wl_display *display, surface_visitor visit, void *data)
{
    visit_request request = {visit, data};
    struct wl_client *client;
    wl_client_for_each(client, wl_display_get_client_list(display)) {
        wl_client_for_each_resource(client, visit_resource, &request);
    }
}

static enum wl_iterator_result visit_resource(struct wl_resource *resource, void *data)
{
    const visit_request *request = data;
    if (strcmp(wl_resource_get_class(resource), wl_surface_interface.name) == 0) {
        request->visit(wlr_surface_from_resource(resource), request->data);
    }
    return WL_ITERATOR_CONTINUE;
}
