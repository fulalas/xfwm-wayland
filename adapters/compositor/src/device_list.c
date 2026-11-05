#include <stdlib.h>
#include <wlr/backend.h>
#include <wlr/backend/libinput.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/util/log.h>

#include "device_list_private.h"
#include "listener.h"
#include "resource.h"
#include "xfce-input-device-list-private-v1-protocol.h"

enum {
    DEVICE_LIST_VERSION = 1,
};

struct device_list {
    compositor *server;
    struct wl_global *global;
    struct wl_list managers;
    struct wl_list entries;
    struct wl_event_source *announcement;
    struct wl_listener new_input;
};

static void bind_list(struct wl_client *client, void *data, uint32_t version, uint32_t id);
static void handle_new_input(struct wl_listener *listener, void *data);
static listed_device *find_entry(device_list *list, struct libinput_device *handle);
static listed_device *add_entry(device_list *list, struct libinput_device *handle);
static void schedule_announcement(device_list *list);
static void announce_new_devices(void *data);
static bool announce_pending(device_list *list, struct wl_resource *manager);

static const struct xfce_input_device_list_private_v1_interface list_implementation = {
    .destroy = resource_handle_destroy,
};

device_list *device_list_create(compositor *server)
{
    device_list *list = calloc(1, sizeof(*list));
    if (list == NULL) {
        return NULL;
    }
    list->server = server;
    wl_list_init(&list->managers);
    wl_list_init(&list->entries);
    list->global = wl_global_create(server->display, &xfce_input_device_list_private_v1_interface,
                                    DEVICE_LIST_VERSION, list, bind_list);
    if (list->global == NULL) {
        wlr_log(WLR_ERROR, "cannot create the input device list global");
        free(list);
        return NULL;
    }
    listener_connect(&list->new_input, &server->backend->events.new_input, handle_new_input);
    return list;
}

void device_list_destroy(device_list *list)
{
    listener_disconnect(&list->new_input);
    if (list->announcement != NULL) {
        wl_event_source_remove(list->announcement);
    }
    listed_device *entry;
    listed_device *tmp;
    wl_list_for_each_safe(entry, tmp, &list->entries, link) {
        listed_device_remove(entry);
    }
    struct wl_resource *manager;
    struct wl_resource *next;
    wl_resource_for_each_safe(manager, next, &list->managers) {
        resource_detach(manager);
    }
    wl_global_destroy(list->global);
    free(list);
}

void device_list_refresh(device_list *list)
{
    listed_device *entry;
    wl_list_for_each(entry, &list->entries, link) {
        if (entry->announced) {
            listed_device_refresh(entry);
        }
    }
}

static void bind_list(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    device_list *list = data;
    struct wl_resource *manager =
        wl_resource_create(client, &xfce_input_device_list_private_v1_interface, version, id);
    if (manager == NULL) {
        wl_client_post_no_memory(client);
        return;
    }
    wl_resource_set_implementation(manager, &list_implementation, list, resource_unlink);
    wl_list_insert(&list->managers, wl_resource_get_link(manager));
    listed_device *entry;
    wl_list_for_each_reverse(entry, &list->entries, link) {
        if (entry->announced) {
            listed_device_announce(entry, manager);
        }
    }
    xfce_input_device_list_private_v1_send_done(manager);
}

static void handle_new_input(struct wl_listener *listener, void *data)
{
    device_list *list = wl_container_of(listener, list, new_input);
    struct wlr_input_device *device = data;
    if (!wlr_input_device_is_libinput(device)) {
        return;
    }
    struct libinput_device *handle = wlr_libinput_get_device_handle(device);
    listed_device *entry = find_entry(list, handle);
    if (entry == NULL) {
        entry = add_entry(list, handle);
    }
    if (entry != NULL) {
        listed_device_add_user(entry, device);
        schedule_announcement(list);
    }
}

static listed_device *find_entry(device_list *list, struct libinput_device *handle)
{
    listed_device *entry;
    wl_list_for_each(entry, &list->entries, link) {
        if (entry->handle == handle) {
            return entry;
        }
    }
    return NULL;
}

static listed_device *add_entry(device_list *list, struct libinput_device *handle)
{
    listed_device *entry = listed_device_create(list, handle);
    if (entry != NULL) {
        wl_list_insert(&list->entries, &entry->link);
    }
    return entry;
}

static void schedule_announcement(device_list *list)
{
    if (list->announcement == NULL) {
        list->announcement =
            wl_event_loop_add_idle(list->server->loop, announce_new_devices, list);
    }
}

static void announce_new_devices(void *data)
{
    device_list *list = data;
    list->announcement = NULL;
    listed_device *entry;
    wl_list_for_each(entry, &list->entries, link) {
        if (!entry->announced) {
            entry->settings = device_settings_read(entry->handle);
        }
    }
    struct wl_resource *manager;
    wl_resource_for_each(manager, &list->managers) {
        if (announce_pending(list, manager)) {
            xfce_input_device_list_private_v1_send_done(manager);
        }
    }
    wl_list_for_each(entry, &list->entries, link) {
        entry->announced = true;
    }
}

static bool announce_pending(device_list *list, struct wl_resource *manager)
{
    bool any = false;
    listed_device *entry;
    wl_list_for_each_reverse(entry, &list->entries, link) {
        if (!entry->announced) {
            listed_device_announce(entry, manager);
            any = true;
        }
    }
    return any;
}
