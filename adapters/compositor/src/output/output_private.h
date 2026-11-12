#ifndef COMPOSITOR_OUTPUT_PRIVATE_H
#define COMPOSITOR_OUTPUT_PRIVATE_H

#include <stdint.h>
#include <wayland-server-core.h>
#include <wlr/types/wlr_scene.h>

#include "output.h"
#include "output_display.h"
#include "render/paint_stats.h"
#include "render/repaint_timing.h"

struct wlr_buffer;
struct wlr_color_transform;
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
    int failures;
} zoom_target;

typedef struct output_brightness {
    struct wlr_color_transform *curve;
    int curve_percent;
    int on_screen;
    int failed_percent;
} output_brightness;

typedef struct output_repaint {
    struct wl_event_source *timer;
    bool armed;
    int64_t wake_due_ns;
    int64_t wake_lateness_ns;
    render_history history;
    struct wlr_scene_timer render_timer;
} output_repaint;

struct output {
    compositor *server;
    struct wlr_output *wlr_output;
    struct wlr_scene_output *scene_output;
    bool scene_hdr;
    wm_rect usable;
    paint_stats stats;
    zoom_target zoom;
    output_repaint repaint;
    output_display display;
    output_brightness brightness;
    struct wl_list link;
    struct wl_listener frame;
    struct wl_listener commit;
    struct wl_listener request_state;
    struct wl_listener destroy;
    struct wl_listener scene_destroy;
};

struct outputs {
    compositor *server;
    struct wl_list list;
    double max_scale;
    bool can_time_renders;
    bool renders_on_cpu;
    bool changed;
    struct wl_event_source *arrange_idle;
    struct wl_listener new_output;
    struct wl_listener layout_change;
};

bool output_attach_auto(output *output);
void output_render(output *output);
void output_render_frame(output *output);
void output_send_frame_done(output *output);
void output_tell_scene_color(const output *output, struct wlr_output_state *state);
bool output_wants_tearing(const output *output);
bool output_commit_state(output *output, struct wlr_output_state *state);
void outputs_arrange(outputs *outputs);

#endif
