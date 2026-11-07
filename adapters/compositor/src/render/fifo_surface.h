#ifndef COMPOSITOR_FIFO_SURFACE_H
#define COMPOSITOR_FIFO_SURFACE_H

#include <wayland-server-core.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/util/addon.h>

#include "present_sync_private.h"

typedef struct fifo_state {
    bool set_barrier;
} fifo_state;

typedef struct fifo_surface {
    present_sync *sync;
    struct wlr_surface *surface;
    struct wl_resource *resource;
    fifo_state pending;
    fifo_state current;
    bool wait_barrier;
    fifo_counters counters;
    struct wl_list waiters;
    struct wl_list link;
    struct wl_event_source *watchdog;
    struct wlr_addon addon;
    struct wlr_surface_synced synced;
    struct wl_listener client_commit;
} fifo_surface;

fifo_surface *fifo_surface_get(present_sync *sync, struct wlr_surface *surface);
fifo_surface *fifo_surface_create(present_sync *sync, struct wlr_surface *surface);

#endif
