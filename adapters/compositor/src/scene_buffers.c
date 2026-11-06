#include <time.h>
#include <wlr/types/wlr_scene.h>

#include "scene_buffers.h"

enum {
    PERCENT = 100,
};

static void set_buffer_opacity(struct wlr_scene_buffer *buffer, int sx, int sy, void *data);

double scene_opacity_from_percent(int percent)
{
    return (double) percent / PERCENT;
}

void scene_buffers_set_opacity(struct wlr_scene_tree *tree, float opacity)
{
    wlr_scene_node_for_each_buffer(&tree->node, set_buffer_opacity, &opacity);
}

bool scene_buffer_reject_input(struct wlr_scene_buffer *buffer, double *sx, double *sy)
{
    (void) buffer;
    (void) sx;
    (void) sy;
    return false;
}

static void set_buffer_opacity(struct wlr_scene_buffer *buffer, int sx, int sy, void *data)
{
    (void) sx;
    (void) sy;
    const float *opacity = data;
    wlr_scene_buffer_set_opacity(buffer, *opacity);
}

void scene_output_send_frame_done_now(struct wlr_scene_output *scene_output)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    wlr_scene_output_send_frame_done(scene_output, &now);
}
