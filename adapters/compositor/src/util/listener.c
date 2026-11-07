#include "listener.h"

void listener_connect(struct wl_listener *listener, struct wl_signal *signal,
                      wl_notify_func_t notify)
{
    listener->notify = notify;
    wl_signal_add(signal, listener);
}

void listener_disconnect(struct wl_listener *listener)
{
    if (listener->link.prev == NULL || listener->link.next == NULL) {
        return;
    }
    wl_list_remove(&listener->link);
    wl_list_init(&listener->link);
}
