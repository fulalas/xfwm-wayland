#ifndef COMPOSITOR_OUTPUT_PRIVATE_H
#define COMPOSITOR_OUTPUT_PRIVATE_H

#include <stdint.h>
#include <wayland-server-core.h>

#include "output.h"
#include "output_display.h"
#include "paint_stats.h"

struct wlr_buffer;
struct wlr_drm_syncobj_timeline;
struct wlr_scene_output;
struct wlr_swapchain;
struct wlr_output_state;

typedef struct zoom_target {
    struct wlr_swapchain *swapchain;
    struct wlr_buffer *source;
    struct wlr_drm_syncobj_timeline *timeline;
    uint64_t point;
    bool cursor_locked;
    bool view_changed;
} zoom_target;

struct output {
    compositor *server;
    struct wlr_output *wlr_output;
    struct wlr_scene_output *scene_output;
    wm_rect usable;
    paint_stats stats;
    zoom_target zoom;
    output_display display;
    struct wl_list link;
    struct wl_listener frame;
    struct wl_listener request_state;
    struct wl_listener destroy;
    struct wl_listener scene_destroy;
};

struct outputs {
    compositor *server;
    struct wl_list list;
    double max_scale;
    bool changed;
    struct wl_event_source *arrange_idle;
    struct wl_listener new_output;
    struct wl_listener layout_change;
};

bool output_attach_auto(output *output);
void output_render(output *output);
bool output_commit_state(output *output, struct wlr_output_state *state);
void outputs_arrange(outputs *outputs);

#endif
