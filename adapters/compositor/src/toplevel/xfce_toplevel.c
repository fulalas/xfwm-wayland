#include <stdlib.h>
#include <string.h>
#include <wlr/types/wlr_foreign_toplevel_management_v1.h>
#include <wlr/util/log.h>

#include "util/listener.h"
#include "util/resource.h"
#include "xfce-foreign-toplevel-management-private-v1-protocol.h"
#include "xfce_toplevel_private.h"

enum {
    MANAGER_VERSION = 1,
};

static const char WORKSPACE_INTERFACE[] = "ext_workspace_handle_v1";

typedef struct manager_binding {
    xfce_toplevels *toplevels;
    struct wl_listener resource_created;
} manager_binding;

static void bind_manager(struct wl_client *client, void *data, uint32_t version, uint32_t id);
static void handle_get_handle(struct wl_client *client, struct wl_resource *manager, uint32_t id,
                              struct wl_resource *wlr_resource);
static void handle_manager_resource_destroy(struct wl_resource *manager);
static void handle_resource_created(struct wl_listener *listener, void *data);
static void handle_republish(void *data);
static bool is_bound(const toplevel_entry *entry, const struct wl_resource *wlr_resource);
static xfce_handle *create_handle(struct wl_resource *manager, uint32_t id);
static void attach_handle(xfce_handle *handle, toplevel_entry *entry,
                          struct wl_resource *wlr_resource);
static void handle_resource_destroy(struct wl_resource *resource);
static void handle_wlr_destroy(struct wl_listener *listener, void *data);
static void detach_handle(xfce_handle *handle);

static const struct xfce_foreign_toplevel_manager_private_v1_interface manager_impl = {
    .get_xfce_foreign_toplevel_handle = handle_get_handle,
    .destroy = resource_handle_destroy,
};

xfce_toplevels *xfce_toplevels_create(compositor *server)
{
    xfce_toplevels *x = calloc(1, sizeof(*x));
    if (x == NULL) {
        return NULL;
    }
    x->server = server;
    wl_list_init(&x->owner_destroy.link);
    x->global = wl_global_create(server->display,
                                 &xfce_foreign_toplevel_manager_private_v1_interface,
                                 MANAGER_VERSION, x, bind_manager);
    if (x->global == NULL) {
        wlr_log(WLR_ERROR, "cannot create the xfce foreign toplevel global");
        free(x);
        return NULL;
    }
    return x;
}

void xfce_toplevels_destroy(xfce_toplevels *x)
{
    xfce_toplevels_release_highlight(x);
    if (x->republish != NULL) {
        wl_event_source_remove(x->republish);
    }
    wl_global_destroy(x->global);
    free(x);
}

void xfce_toplevels_publish(toplevel_entry *entry)
{
    xfce_handle *handle;
    wl_list_for_each(handle, &entry->xfce_handles, link) {
        xfce_handle_publish(handle);
    }
}

void xfce_toplevels_detach(toplevel_entry *entry)
{
    xfce_handle *handle;
    xfce_handle *tmp;
    wl_list_for_each_safe(handle, tmp, &entry->xfce_handles, link) {
        detach_handle(handle);
    }
}

static void bind_manager(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    manager_binding *binding = calloc(1, sizeof(*binding));
    if (binding == NULL) {
        wl_client_post_no_memory(client);
        return;
    }
    binding->toplevels = data;
    if (resource_bind(client, &xfce_foreign_toplevel_manager_private_v1_interface, version, id,
                      &manager_impl, binding, handle_manager_resource_destroy) == NULL) {
        free(binding);
        return;
    }
    binding->resource_created.notify = handle_resource_created;
    wl_client_add_resource_created_listener(client, &binding->resource_created);
}

static void handle_get_handle(struct wl_client *client, struct wl_resource *manager, uint32_t id,
                              struct wl_resource *wlr_resource)
{
    struct wlr_foreign_toplevel_handle_v1 *wlr = wl_resource_get_user_data(wlr_resource);
    toplevel_entry *entry = wlr != NULL ? wlr->data : NULL;
    if (entry != NULL && is_bound(entry, wlr_resource)) {
        wl_resource_post_error(manager,
                               XFCE_FOREIGN_TOPLEVEL_MANAGER_PRIVATE_V1_ERROR_ALREADY_BOUND,
                               "the toplevel handle already has an xfce handle");
        return;
    }
    xfce_handle *handle = create_handle(manager, id);
    if (handle == NULL) {
        wl_client_post_no_memory(client);
        return;
    }
    if (entry != NULL) {
        attach_handle(handle, entry, wlr_resource);
    }
}

static void handle_manager_resource_destroy(struct wl_resource *manager)
{
    manager_binding *binding = wl_resource_get_user_data(manager);
    listener_disconnect(&binding->resource_created);
    free(binding);
}

static void handle_resource_created(struct wl_listener *listener, void *data)
{
    manager_binding *binding = wl_container_of(listener, binding, resource_created);
    xfce_toplevels *x = binding->toplevels;
    if (strcmp(wl_resource_get_class(data), WORKSPACE_INTERFACE) != 0 || x->republish != NULL) {
        return;
    }
    x->republish = wl_event_loop_add_idle(x->server->loop, handle_republish, x);
}

static void handle_republish(void *data)
{
    xfce_toplevels *x = data;
    x->republish = NULL;
    if (x->server->foreign_toplevels != NULL) {
        foreign_toplevels_sync_all(x->server->foreign_toplevels);
    }
}

static bool is_bound(const toplevel_entry *entry, const struct wl_resource *wlr_resource)
{
    const xfce_handle *handle;
    wl_list_for_each(handle, &entry->xfce_handles, link) {
        if (handle->wlr_resource == wlr_resource) {
            return true;
        }
    }
    return false;
}

static xfce_handle *create_handle(struct wl_resource *manager, uint32_t id)
{
    xfce_handle *handle = calloc(1, sizeof(*handle));
    if (handle == NULL) {
        return NULL;
    }
    handle->resource = wl_resource_create(wl_resource_get_client(manager),
                                          &xfce_foreign_toplevel_handle_v1_interface,
                                          wl_resource_get_version(manager), id);
    if (handle->resource == NULL) {
        free(handle);
        return NULL;
    }
    const manager_binding *binding = wl_resource_get_user_data(manager);
    handle->toplevels = binding->toplevels;
    wl_list_init(&handle->link);
    wl_list_init(&handle->wlr_destroy.link);
    wl_list_init(&handle->workspace_destroy.link);
    wl_resource_set_implementation(handle->resource, xfce_handle_requests(), handle,
                                   handle_resource_destroy);
    return handle;
}

static void attach_handle(xfce_handle *handle, toplevel_entry *entry,
                          struct wl_resource *wlr_resource)
{
    handle->entry = entry;
    handle->wlr_resource = wlr_resource;
    handle->published.states = UINT32_MAX;
    wl_list_insert(entry->xfce_handles.prev, &handle->link);
    handle->wlr_destroy.notify = handle_wlr_destroy;
    wl_resource_add_destroy_listener(handle->wlr_resource, &handle->wlr_destroy);
    xfce_handle_publish(handle);
}

static void handle_resource_destroy(struct wl_resource *resource)
{
    xfce_handle *handle = wl_resource_get_user_data(resource);
    detach_handle(handle);
    free(handle);
}

static void handle_wlr_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    xfce_handle *handle = wl_container_of(listener, handle, wlr_destroy);
    detach_handle(handle);
}

static void detach_handle(xfce_handle *handle)
{
    listener_disconnect(&handle->wlr_destroy);
    xfce_handle_forget_workspace(handle);
    wl_list_remove(&handle->link);
    wl_list_init(&handle->link);
    handle->entry = NULL;
    handle->wlr_resource = NULL;
}
