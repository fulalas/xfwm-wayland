#ifndef COMPOSITOR_SCENE_BUFFERS_H
#define COMPOSITOR_SCENE_BUFFERS_H

#include <stdbool.h>

struct wlr_scene_buffer;
struct wlr_scene_output;
struct wlr_scene_tree;

double scene_opacity_from_percent(int percent);
void scene_buffers_set_opacity(struct wlr_scene_tree *tree, float opacity);
bool scene_buffer_reject_input(struct wlr_scene_buffer *buffer, double *sx, double *sy);
void scene_output_send_frame_done_now(struct wlr_scene_output *scene_output);

#endif
