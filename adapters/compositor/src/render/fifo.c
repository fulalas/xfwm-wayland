#include "fifo-v1-protocol.h"
#include "fifo_surface.h"
#include "util/resource.h"

enum {
    FIFO_VERSION = 1,
};

static void bind_manager(struct wl_client *client, void *data, uint32_t version, uint32_t id);
static void handle_get_fifo(struct wl_client *client, struct wl_resource *resource, uint32_t id,
                            struct wl_resource *surface_resource);
static void handle_set_barrier(struct wl_client *client, struct wl_resource *resource);
static void handle_wait_barrier(struct wl_client *client, struct wl_resource *resource);
static void handle_resource_destroy(struct wl_resource *resource);
static fifo_surface *fifo_from_resource(struct wl_resource *resource);

static const struct wp_fifo_manager_v1_interface manager_impl = {
    .destroy = resource_handle_destroy,
    .get_fifo = handle_get_fifo,
};

static const struct wp_fifo_v1_interface fifo_impl = {
    .set_barrier = handle_set_barrier,
    .wait_barrier = handle_wait_barrier,
    .destroy = resource_handle_destroy,
};

bool fifo_global_create(present_sync *sync)
{
    struct wl_display *display = sync->server->display;
    sync->fifo_global =
        wl_global_create(display, &wp_fifo_manager_v1_interface, FIFO_VERSION, sync, bind_manager);
    return sync->fifo_global != NULL;
}

static void bind_manager(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    resource_bind(client, &wp_fifo_manager_v1_interface, version, id, &manager_impl, data, NULL);
}

static void handle_get_fifo(struct wl_client *client, struct wl_resource *resource, uint32_t id,
                            struct wl_resource *surface_resource)
{
    present_sync *sync = wl_resource_get_user_data(resource);
    struct wlr_surface *surface = wlr_surface_from_resource(surface_resource);
    fifo_surface *fs = fifo_surface_get(sync, surface);
    if (fs != NULL && fs->resource != NULL) {
        wl_resource_post_error(resource, WP_FIFO_MANAGER_V1_ERROR_ALREADY_EXISTS,
                               "a fifo object already exists for this surface");
        return;
    }
    if (fs == NULL) {
        fs = fifo_surface_create(sync, surface);
    }
    int version = wl_resource_get_version(resource);
    struct wl_resource *fifo =
        fs != NULL ? wl_resource_create(client, &wp_fifo_v1_interface, version, id) : NULL;
    if (fifo == NULL) {
        wl_resource_post_no_memory(resource);
        return;
    }
    wl_resource_set_implementation(fifo, &fifo_impl, fs, handle_resource_destroy);
    fs->resource = fifo;
}

static void handle_set_barrier(struct wl_client *client, struct wl_resource *resource)
{
    (void) client;
    fifo_surface *fs = fifo_from_resource(resource);
    if (fs != NULL) {
        fs->pending.set_barrier = true;
    }
}

static void handle_wait_barrier(struct wl_client *client, struct wl_resource *resource)
{
    (void) client;
    fifo_surface *fs = fifo_from_resource(resource);
    if (fs != NULL) {
        fs->wait_barrier = true;
    }
}

static void handle_resource_destroy(struct wl_resource *resource)
{
    fifo_surface *fs = wl_resource_get_user_data(resource);
    if (fs != NULL) {
        fs->resource = NULL;
    }
}

static fifo_surface *fifo_from_resource(struct wl_resource *resource)
{
    fifo_surface *fs = wl_resource_get_user_data(resource);
    if (fs == NULL) {
        wl_resource_post_error(resource, WP_FIFO_V1_ERROR_SURFACE_DESTROYED,
                               "the surface of this fifo object was destroyed");
    }
    return fs;
}
