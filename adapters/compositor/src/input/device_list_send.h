#ifndef COMPOSITOR_DEVICE_LIST_SEND_H
#define COMPOSITOR_DEVICE_LIST_SEND_H

#include "device_settings.h"

struct wl_resource;

void device_list_send_all(struct wl_resource *resource, const device_settings *settings);
void device_list_send_changes(struct wl_resource *resource, const device_settings *before,
                              const device_settings *after);

#endif
