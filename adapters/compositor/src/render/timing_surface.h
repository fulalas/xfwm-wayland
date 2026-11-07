#ifndef COMPOSITOR_TIMING_SURFACE_H
#define COMPOSITOR_TIMING_SURFACE_H

#include <wayland-server-core.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/util/addon.h>

#include "present_sync_private.h"

typedef struct timing_surface {
    present_sync *sync;
    struct wlr_surface *surface;
    struct wl_resource *resource;
    bool has_target;
    int64_t target_ns;
    struct wl_list waiters;
    struct wl_list link;
    struct wl_event_source *timer;
    struct wlr_addon addon;
    struct wl_listener client_commit;
} timing_surface;

timing_surface *timing_surface_get(present_sync *sync, struct wlr_surface *surface);
timing_surface *timing_surface_create(present_sync *sync, struct wlr_surface *surface);

#endif
