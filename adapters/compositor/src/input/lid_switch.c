#include "lid_switch.h"

#include <stdlib.h>
#include <wlr/backend.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_switch.h>

#include "output/displays.h"
#include "util/listener.h"

typedef struct lid_device {
    lid_switch *owner;
    struct wl_list link;
    struct wl_listener toggle;
    struct wl_listener destroy;
} lid_device;

struct lid_switch {
    compositor *server;
    struct wl_list devices;
    struct wl_listener new_input;
};

static void handle_new_input(struct wl_listener *listener, void *data);
static void handle_toggle(struct wl_listener *listener, void *data);
static void handle_device_destroy(struct wl_listener *listener, void *data);
static void device_destroy(lid_device *device);

lid_switch *lid_switch_create(compositor *server)
{
    lid_switch *lid = calloc(1, sizeof(*lid));
    if (lid == NULL) {
        return NULL;
    }
    lid->server = server;
    wl_list_init(&lid->devices);
    listener_connect(&lid->new_input, &server->backend->events.new_input, handle_new_input);
    return lid;
}

void lid_switch_destroy(lid_switch *lid)
{
    lid_device *device;
    lid_device *tmp;
    wl_list_for_each_safe(device, tmp, &lid->devices, link) {
        device_destroy(device);
    }
    listener_disconnect(&lid->new_input);
    free(lid);
}

static void handle_new_input(struct wl_listener *listener, void *data)
{
    lid_switch *lid = wl_container_of(listener, lid, new_input);
    struct wlr_input_device *input = data;
    if (input->type != WLR_INPUT_DEVICE_SWITCH) {
        return;
    }
    lid_device *device = calloc(1, sizeof(*device));
    if (device == NULL) {
        return;
    }
    device->owner = lid;
    struct wlr_switch *wlr_switch = wlr_switch_from_input_device(input);
    listener_connect(&device->toggle, &wlr_switch->events.toggle, handle_toggle);
    listener_connect(&device->destroy, &input->events.destroy, handle_device_destroy);
    wl_list_insert(&lid->devices, &device->link);
}

static void handle_toggle(struct wl_listener *listener, void *data)
{
    lid_device *device = wl_container_of(listener, device, toggle);
    const struct wlr_switch_toggle_event *event = data;
    if (event->switch_type != WLR_SWITCH_TYPE_LID) {
        return;
    }
    bool closed = event->switch_state == WLR_SWITCH_STATE_ON;
    displays_set_lid_closed(device->owner->server->displays, closed);
}

static void handle_device_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    lid_device *device = wl_container_of(listener, device, destroy);
    device_destroy(device);
}

static void device_destroy(lid_device *device)
{
    listener_disconnect(&device->toggle);
    listener_disconnect(&device->destroy);
    wl_list_remove(&device->link);
    free(device);
}
