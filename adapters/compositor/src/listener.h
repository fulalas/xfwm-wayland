#ifndef COMPOSITOR_LISTENER_H
#define COMPOSITOR_LISTENER_H

#include <wayland-server-core.h>

void listener_connect(struct wl_listener *listener, struct wl_signal *signal,
                      wl_notify_func_t notify);
void listener_disconnect(struct wl_listener *listener);

#endif
