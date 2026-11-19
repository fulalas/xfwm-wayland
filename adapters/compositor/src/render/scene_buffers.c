#include <time.h>
#include <wlr/types/wlr_scene.h>

#include "scene_buffers.h"
#include "util/node_owner.h"

enum {
    PERCENT = 100,
};

static void set_buffer_opacity(struct wlr_scene_buffer *buffer, int sx, int sy, void *data);
static void send_frame_done(struct wlr_scene_node *node, struct wlr_scene_output *scene_output,
                            const struct timespec *now);

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
    send_frame_done(&scene_output->scene->tree.node, scene_output, &now);
}

static void send_frame_done(struct wlr_scene_node *node, struct wlr_scene_output *scene_output,
                            const struct timespec *now)
{
    if (!node->enabled || node_owner_is_decor(node)) {
        return;
    }
    if (node->type == WLR_SCENE_NODE_BUFFER) {
        struct wlr_scene_frame_done_event event = {.output = scene_output, .when = *now};
        wlr_scene_buffer_send_frame_done(wlr_scene_buffer_from_node(node), &event);
        return;
    }
    if (node->type != WLR_SCENE_NODE_TREE) {
        return;
    }
    struct wlr_scene_tree *tree = wlr_scene_tree_from_node(node);
    struct wlr_scene_node *child;
    wl_list_for_each(child, &tree->children, link) {
        send_frame_done(child, scene_output, now);
    }
}
