#include <stdlib.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>

#include "util/listener.h"
#include "util/resource.h"
#include "xdg-output-unstable-v1-protocol.h"
#include "xdg_output.h"

enum {
    MANAGER_VERSION = 3,
    DONE_DEPRECATED_SINCE_VERSION = 3,
};

struct xdg_outputs {
    struct wl_global *global;
    struct wlr_output_layout *layout;
    xdg_output_scaler scaler;
    struct wl_list handles;
    struct wl_listener layout_change;
    struct wl_listener layout_destroy;
    struct wl_listener display_destroy;
};

typedef struct output_handle {
    xdg_outputs *owner;
    struct wl_resource *resource;
    struct wlr_output *output;
    bool has_sent;
    wm_rect sent;
    struct wl_list link;
    struct wl_listener output_destroy;
} output_handle;

static void bind_manager(struct wl_client *client, void *data, uint32_t version, uint32_t id);
static void handle_get_xdg_output(struct wl_client *client, struct wl_resource *resource,
                                  uint32_t id, struct wl_resource *output_resource);
static void handle_resource_destroy(struct wl_resource *resource);
static void handle_output_destroy(struct wl_listener *listener, void *data);
static void handle_layout_change(struct wl_listener *listener, void *data);
static void handle_layout_destroy(struct wl_listener *listener, void *data);
static void handle_display_destroy(struct wl_listener *listener, void *data);
static void send_identity(const output_handle *handle);
static bool send_box(output_handle *handle);
static bool logical_box(const output_handle *handle, wm_rect *box);
static void detach(output_handle *handle);

static const struct zxdg_output_manager_v1_interface manager_impl = {
    .destroy = resource_handle_destroy,
    .get_xdg_output = handle_get_xdg_output,
};

static const struct zxdg_output_v1_interface output_impl = {
    .destroy = resource_handle_destroy,
};

xdg_outputs *xdg_outputs_create(struct wl_display *display, struct wlr_output_layout *layout,
                                const xdg_output_scaler *scaler)
{
    xdg_outputs *outputs = calloc(1, sizeof(*outputs));
    if (outputs == NULL) {
        return NULL;
    }
    outputs->global = wl_global_create(display, &zxdg_output_manager_v1_interface,
                                       MANAGER_VERSION, outputs, bind_manager);
    if (outputs->global == NULL) {
        free(outputs);
        return NULL;
    }
    outputs->layout = layout;
    outputs->scaler = *scaler;
    wl_list_init(&outputs->handles);
    listener_connect(&outputs->layout_change, &layout->events.change, handle_layout_change);
    listener_connect(&outputs->layout_destroy, &layout->events.destroy, handle_layout_destroy);
    outputs->display_destroy.notify = handle_display_destroy;
    wl_display_add_destroy_listener(display, &outputs->display_destroy);
    return outputs;
}

void xdg_outputs_refresh(xdg_outputs *outputs)
{
    output_handle *handle;
    wl_list_for_each(handle, &outputs->handles, link) {
        if (send_box(handle)) {
            wlr_output_schedule_done(handle->output);
        }
    }
}

static void bind_manager(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    struct wl_resource *resource =
        wl_resource_create(client, &zxdg_output_manager_v1_interface, (int) version, id);
    if (resource == NULL) {
        wl_client_post_no_memory(client);
        return;
    }
    wl_resource_set_implementation(resource, &manager_impl, data, NULL);
}

static void handle_get_xdg_output(struct wl_client *client, struct wl_resource *resource,
                                  uint32_t id, struct wl_resource *output_resource)
{
    xdg_outputs *outputs = wl_resource_get_user_data(resource);
    int version = wl_resource_get_version(resource);
    output_handle *handle = calloc(1, sizeof(*handle));
    struct wl_resource *xdg_resource =
        handle != NULL ? wl_resource_create(client, &zxdg_output_v1_interface, version, id) : NULL;
    if (xdg_resource == NULL) {
        free(handle);
        wl_client_post_no_memory(client);
        return;
    }
    handle->owner = outputs;
    handle->resource = xdg_resource;
    handle->output = wlr_output_from_resource(output_resource);
    wl_list_init(&handle->link);
    wl_list_init(&handle->output_destroy.link);
    wl_resource_set_implementation(xdg_resource, &output_impl, handle, handle_resource_destroy);
    if (handle->output == NULL) {
        return;
    }
    wl_list_insert(&outputs->handles, &handle->link);
    listener_connect(&handle->output_destroy, &handle->output->events.destroy,
                     handle_output_destroy);
    send_identity(handle);
    bool sent = send_box(handle);
    if (sent && version >= DONE_DEPRECATED_SINCE_VERSION
        && wl_resource_get_version(output_resource) >= WL_OUTPUT_DONE_SINCE_VERSION) {
        wl_output_send_done(output_resource);
    }
}

static void handle_resource_destroy(struct wl_resource *resource)
{
    output_handle *handle = wl_resource_get_user_data(resource);
    detach(handle);
    free(handle);
}

static void handle_output_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    output_handle *handle = wl_container_of(listener, handle, output_destroy);
    detach(handle);
}

static void handle_layout_change(struct wl_listener *listener, void *data)
{
    (void) data;
    xdg_outputs *outputs = wl_container_of(listener, outputs, layout_change);
    xdg_outputs_refresh(outputs);
}

static void handle_layout_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    xdg_outputs *outputs = wl_container_of(listener, outputs, layout_destroy);
    listener_disconnect(&outputs->layout_change);
    listener_disconnect(&outputs->layout_destroy);
    outputs->layout = NULL;
}

static void handle_display_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    xdg_outputs *outputs = wl_container_of(listener, outputs, display_destroy);
    output_handle *handle;
    output_handle *next;
    wl_list_for_each_safe(handle, next, &outputs->handles, link) {
        detach(handle);
    }
    listener_disconnect(&outputs->layout_change);
    listener_disconnect(&outputs->layout_destroy);
    wl_list_remove(&outputs->display_destroy.link);
    wl_global_destroy(outputs->global);
    free(outputs);
}

static void send_identity(const output_handle *handle)
{
    int version = wl_resource_get_version(handle->resource);
    if (version >= ZXDG_OUTPUT_V1_NAME_SINCE_VERSION) {
        zxdg_output_v1_send_name(handle->resource, handle->output->name);
    }
    if (version >= ZXDG_OUTPUT_V1_DESCRIPTION_SINCE_VERSION && handle->output->description) {
        zxdg_output_v1_send_description(handle->resource, handle->output->description);
    }
}

static bool send_box(output_handle *handle)
{
    wm_rect box;
    if (!logical_box(handle, &box) || (handle->has_sent && wm_rect_equals(&box, &handle->sent))) {
        return false;
    }
    handle->has_sent = true;
    handle->sent = box;
    zxdg_output_v1_send_logical_position(handle->resource, box.x, box.y);
    zxdg_output_v1_send_logical_size(handle->resource, box.width, box.height);
    if (wl_resource_get_version(handle->resource) < DONE_DEPRECATED_SINCE_VERSION) {
        zxdg_output_v1_send_done(handle->resource);
    }
    return true;
}

static bool logical_box(const output_handle *handle, wm_rect *box)
{
    struct wlr_box placed = {0};
    if (handle->owner->layout != NULL) {
        wlr_output_layout_get_box(handle->owner->layout, handle->output, &placed);
    }
    if (wlr_box_empty(&placed)) {
        return false;
    }
    const xdg_output_scaler *scaler = &handle->owner->scaler;
    wm_rect logical = {placed.x, placed.y, placed.width, placed.height};
    *box = scaler->client_box(scaler->self, wl_resource_get_client(handle->resource), logical);
    return true;
}

static void detach(output_handle *handle)
{
    listener_disconnect(&handle->output_destroy);
    wl_list_remove(&handle->link);
    wl_list_init(&handle->link);
    handle->output = NULL;
}
