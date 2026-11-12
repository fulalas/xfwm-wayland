#ifndef COMPOSITOR_DEVICE_LIST_PRIVATE_H
#define COMPOSITOR_DEVICE_LIST_PRIVATE_H

#include <wayland-server-core.h>

#include "device_list.h"
#include "device_settings.h"

struct libinput_device;
struct wlr_input_device;

typedef struct listed_device {
    struct libinput_device *handle;
    uint32_t capabilities;
    device_settings settings;
    bool announced;
    struct wl_list users;
    struct wl_list resources;
    struct wl_list link;
} listed_device;

listed_device *listed_device_create(struct libinput_device *handle);
void listed_device_add_user(listed_device *entry, struct wlr_input_device *device);
void listed_device_announce(listed_device *entry, struct wl_resource *manager);
void listed_device_refresh(listed_device *entry);
void listed_device_remove(listed_device *entry);

#endif
