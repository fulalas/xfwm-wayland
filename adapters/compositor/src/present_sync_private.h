#ifndef COMPOSITOR_PRESENT_SYNC_PRIVATE_H
#define COMPOSITOR_PRESENT_SYNC_PRIVATE_H

#include <wayland-server-core.h>

#include "present_rules.h"
#include "present_sync.h"

struct wlr_output;
struct wlr_surface;

typedef struct output_clock {
    present_sync *sync;
    struct wlr_output *output;
    vblank_clock clock;
    struct wl_list link;
    struct wl_listener commit;
    struct wl_listener present;
    struct wl_listener frame;
    struct wl_listener destroy;
} output_clock;

struct present_sync {
    compositor *server;
    struct wl_event_loop *loop;
    struct wl_list outputs;
    struct wl_list fifo_surfaces;
    struct wl_list timing_surfaces;
    struct wl_global *fifo_global;
    struct wl_global *timing_global;
    struct wl_event_source *fifo_idle;
    struct wl_listener new_output;
};

bool fifo_global_create(present_sync *sync);
void fifo_output_latched(present_sync *sync, struct wlr_output *output);

bool commit_timing_global_create(present_sync *sync);
void commit_timing_output_frame(present_sync *sync, struct wlr_output *output);

const vblank_clock *present_sync_clock(const present_sync *sync, const struct wlr_output *output);
bool present_surface_on_output(struct wlr_surface *surface, const struct wlr_output *output);
void present_surface_schedule_frames(struct wlr_surface *surface);

#endif
