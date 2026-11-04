#include <stdlib.h>
#include <wlr/util/log.h>

#include "accessibility.h"
#include "osd.h"
#include "seat_resource.h"
#include "xfce-accessibility-private-v1-protocol.h"

enum {
    MANAGER_VERSION = 1,
};

struct accessibility {
    compositor *server;
    struct wl_global *global;
};

static void bind_manager(struct wl_client *client, void *data, uint32_t version, uint32_t id);
static void handle_find_pointer(struct wl_client *client, struct wl_resource *resource,
                                struct wl_resource *seat);
static bool find_cursor_enabled(const compositor *server);

static const struct xfce_accessibility_manager_private_v1_interface manager_impl = {
    .find_pointer = handle_find_pointer,
};

accessibility *accessibility_create(compositor *server)
{
    accessibility *a = calloc(1, sizeof(*a));
    if (a == NULL) {
        return NULL;
    }
    a->server = server;
    a->global = wl_global_create(server->display, &xfce_accessibility_manager_private_v1_interface,
                                 MANAGER_VERSION, a, bind_manager);
    if (a->global == NULL) {
        wlr_log(WLR_ERROR, "cannot create the xfce accessibility global");
        free(a);
        return NULL;
    }
    return a;
}

void accessibility_destroy(accessibility *a)
{
    wl_global_destroy(a->global);
    free(a);
}

static void bind_manager(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    struct wl_resource *resource = wl_resource_create(
        client, &xfce_accessibility_manager_private_v1_interface, (int) version, id);
    if (resource == NULL) {
        wl_client_post_no_memory(client);
        return;
    }
    wl_resource_set_implementation(resource, &manager_impl, data, NULL);
}

static void handle_find_pointer(struct wl_client *client, struct wl_resource *resource,
                                struct wl_resource *seat)
{
    (void) client;
    accessibility *a = wl_resource_get_user_data(resource);
    if (seat_resource_is_ours(a->server, seat) && find_cursor_enabled(a->server)) {
        osd_show_find_cursor(a->server->osd);
    }
}

static bool find_cursor_enabled(const compositor *server)
{
    const compositor_services *services = server->services;
    return services->accessibility_prefs(services->self).find_cursor;
}
