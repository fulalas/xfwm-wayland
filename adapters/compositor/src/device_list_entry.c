#include <libinput.h>
#include <stdlib.h>
#include <wlr/types/wlr_input_device.h>

#include "device_list_private.h"
#include "device_list_send.h"
#include "listener.h"
#include "resource.h"
#include "xfce-input-device-list-private-v1-protocol.h"

typedef struct device_user {
    listed_device *entry;
    struct wl_list link;
    struct wl_listener destroy;
} device_user;

static void handle_user_destroy(struct wl_listener *listener, void *data);
static void free_user(device_user *user);
static void handle_device_gone(struct wl_resource *resource);
static void detach_resource(struct wl_resource *resource);

static const struct xfce_input_device_v1_interface device_implementation = {
    .release = resource_handle_destroy,
};

listed_device *listed_device_create(device_list *list, struct libinput_device *handle)
{
    listed_device *entry = calloc(1, sizeof(*entry));
    if (entry == NULL) {
        return NULL;
    }
    entry->list = list;
    entry->handle = libinput_device_ref(handle);
    entry->capabilities = device_capabilities(handle);
    wl_list_init(&entry->users);
    wl_list_init(&entry->resources);
    return entry;
}

void listed_device_add_user(listed_device *entry, struct wlr_input_device *device)
{
    device_user *user = calloc(1, sizeof(*user));
    if (user == NULL) {
        return;
    }
    user->entry = entry;
    wl_list_insert(&entry->users, &user->link);
    listener_connect(&user->destroy, &device->events.destroy, handle_user_destroy);
}

void listed_device_announce(listed_device *entry, struct wl_resource *manager)
{
    struct wl_client *client = wl_resource_get_client(manager);
    struct wl_resource *resource = wl_resource_create(
        client, &xfce_input_device_v1_interface, wl_resource_get_version(manager), 0);
    if (resource == NULL) {
        wl_client_post_no_memory(client);
        return;
    }
    wl_resource_set_implementation(resource, &device_implementation, entry, handle_device_gone);
    wl_list_insert(&entry->resources, wl_resource_get_link(resource));
    xfce_input_device_list_private_v1_send_device(
        manager, resource, libinput_device_get_name(entry->handle), entry->capabilities);
    device_list_send_all(resource, &entry->settings);
}

void listed_device_refresh(listed_device *entry)
{
    device_settings current = device_settings_read(entry->handle);
    struct wl_resource *resource;
    wl_resource_for_each(resource, &entry->resources) {
        device_list_send_changes(resource, &entry->settings, &current);
    }
    entry->settings = current;
}

void listed_device_remove(listed_device *entry)
{
    struct wl_resource *resource;
    struct wl_resource *next;
    wl_resource_for_each_safe(resource, next, &entry->resources) {
        xfce_input_device_v1_send_removed(resource);
        detach_resource(resource);
    }
    device_user *user;
    device_user *tmp;
    wl_list_for_each_safe(user, tmp, &entry->users, link) {
        free_user(user);
    }
    wl_list_remove(&entry->link);
    libinput_device_unref(entry->handle);
    free(entry);
}

static void handle_user_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    device_user *user = wl_container_of(listener, user, destroy);
    listed_device *entry = user->entry;
    free_user(user);
    if (wl_list_empty(&entry->users)) {
        listed_device_remove(entry);
    }
}

static void free_user(device_user *user)
{
    listener_disconnect(&user->destroy);
    wl_list_remove(&user->link);
    free(user);
}

static void handle_device_gone(struct wl_resource *resource)
{
    wl_list_remove(wl_resource_get_link(resource));
}

static void detach_resource(struct wl_resource *resource)
{
    wl_resource_set_user_data(resource, NULL);
    wl_list_remove(wl_resource_get_link(resource));
    wl_list_init(wl_resource_get_link(resource));
}
