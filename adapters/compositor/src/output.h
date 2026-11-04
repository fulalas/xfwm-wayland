#ifndef COMPOSITOR_OUTPUT_H
#define COMPOSITOR_OUTPUT_H

#include <stddef.h>

#include "server.h"
#include "wm/geometry.h"

struct wlr_output;
struct wlr_scene_output;

typedef struct output output;

outputs *outputs_create(compositor *server);
void outputs_destroy(outputs *outputs);
size_t outputs_count(const outputs *outputs);
output *outputs_at_index(const outputs *outputs, size_t index);
size_t outputs_known_count(const outputs *outputs);
output *outputs_known_at(const outputs *outputs, size_t index);
output *outputs_at_point(const outputs *outputs, wm_point point);
output *outputs_near_point(const outputs *outputs, wm_point point);
output *outputs_for_rect(const outputs *outputs, const wm_rect *rect);
double outputs_max_scale(const outputs *outputs);
void outputs_schedule_arrange(outputs *outputs);
void outputs_mark_changed(outputs *outputs);
void outputs_schedule_frames(outputs *outputs);
bool outputs_renderer_changed(outputs *outputs);
bool outputs_rebuild_swapchains(outputs *outputs);
bool output_rebuild_swapchain(output *output);

bool output_is_active(const output *output);
bool output_place(output *output, wm_point position);
void output_remove_from_layout(output *output);
struct wlr_scene_output *output_scene(output *output);
wm_rect output_full(const output *output);
wm_rect output_usable(const output *output);
struct wlr_output *output_wlr(const output *output);
output *output_from_wlr(struct wlr_output *wlr_output);

#endif
