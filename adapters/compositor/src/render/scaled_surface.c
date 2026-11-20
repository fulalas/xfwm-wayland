#include <math.h>
#include <pixman.h>
#include <stdlib.h>
#include <wlr/types/wlr_alpha_modifier_v1.h>
#include <wlr/types/wlr_buffer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_linux_drm_syncobj_v1.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_single_pixel_buffer_v1.h>
#include <wlr/util/addon.h>

#include "scaled_surface.h"
#include "util/listener.h"

typedef struct scaled_surface {
    scaled_surfaces *set;
    struct wlr_scene_buffer *buffer;
    struct wlr_surface *surface;
    struct wlr_addon addon;
    struct wl_list link;
    struct wl_listener commit;
} scaled_surface;

struct scaled_surfaces {
    double scale;
    scaled_surface_sizer sizer;
    struct wl_list members;
};

static void handle_addon_destroy(struct wlr_addon *addon);
static void handle_commit(struct wl_listener *listener, void *data);
static bool accepts_input(struct wlr_scene_buffer *buffer, double *sx, double *sy);
static scaled_surface *member_of(struct wlr_scene_buffer *buffer);
static void to_local(const scaled_surface *member, double *sx, double *sy);
static void show_buffer(scaled_surface *member);
static void set_buffer(scaled_surface *member);
static void schedule_frame(scaled_surface *member);
static struct wlr_output *frame_pacing_output(struct wlr_surface *surface);
static void fit(scaled_surface *member);
static void fit_size(scaled_surface *member);
static void fit_opaque_region(scaled_surface *member, double scale);
static void release_client_buffer(struct wlr_scene_buffer *buffer);
static void forget(scaled_surface *member);

static const struct wlr_addon_interface scaled_surface_addon = {
    .name = "xfwm_scaled_surface",
    .destroy = handle_addon_destroy,
};

scaled_surfaces *scaled_surfaces_create(scaled_surface_sizer sizer)
{
    scaled_surfaces *set = calloc(1, sizeof(*set));
    if (set != NULL) {
        set->scale = 1.0;
        set->sizer = sizer;
        wl_list_init(&set->members);
    }
    return set;
}

void scaled_surfaces_destroy(scaled_surfaces *set)
{
    scaled_surface *member;
    scaled_surface *next;
    wl_list_for_each_safe(member, next, &set->members, link) {
        wlr_addon_finish(&member->addon);
        forget(member);
    }
    free(set);
}

struct wlr_scene_tree *scaled_surfaces_add(scaled_surfaces *set, struct wlr_scene_tree *parent,
                                           struct wlr_surface *surface)
{
    struct wlr_scene_tree *tree = wlr_scene_tree_create(parent);
    if (tree == NULL) {
        return NULL;
    }
    struct wlr_scene_surface *scene_surface = wlr_scene_surface_create(tree, surface);
    scaled_surface *member = scene_surface != NULL ? calloc(1, sizeof(*member)) : NULL;
    if (member == NULL) {
        wlr_scene_node_destroy(&tree->node);
        return NULL;
    }
    member->set = set;
    member->buffer = scene_surface->buffer;
    member->surface = surface;
    wlr_addon_init(&member->addon, &member->buffer->node.addons, member->buffer,
                   &scaled_surface_addon);
    member->buffer->point_accepts_input = accepts_input;
    listener_disconnect(&scene_surface->WLR_PRIVATE.surface_commit);
    listener_connect(&member->commit, &surface->events.commit, handle_commit);
    wl_list_insert(&set->members, &member->link);
    fit(member);
    return tree;
}

void scaled_surfaces_set_scale(scaled_surfaces *set, double scale)
{
    set->scale = scale;
    scaled_surface *member;
    wl_list_for_each(member, &set->members, link) {
        fit(member);
    }
}

void scaled_surfaces_refit(struct wlr_scene_tree *tree)
{
    if (tree == NULL) {
        return;
    }
    struct wlr_scene_node *node;
    wl_list_for_each(node, &tree->children, link) {
        scaled_surface *member = node->type == WLR_SCENE_NODE_BUFFER
                                     ? member_of(wlr_scene_buffer_from_node(node))
                                     : NULL;
        if (member != NULL) {
            fit_size(member);
        }
    }
}

bool scaled_surfaces_to_local(struct wlr_scene_buffer *buffer, double *sx, double *sy)
{
    scaled_surface *member = member_of(buffer);
    if (member == NULL) {
        return false;
    }
    to_local(member, sx, sy);
    return true;
}

static void handle_addon_destroy(struct wlr_addon *addon)
{
    scaled_surface *member = wl_container_of(addon, member, addon);
    wlr_addon_finish(addon);
    forget(member);
}

static void handle_commit(struct wl_listener *listener, void *data)
{
    (void) data;
    scaled_surface *member = wl_container_of(listener, member, commit);
    show_buffer(member);
    schedule_frame(member);
}

static bool accepts_input(struct wlr_scene_buffer *buffer, double *sx, double *sy)
{
    scaled_surface *member = member_of(buffer);
    if (member == NULL) {
        return false;
    }
    to_local(member, sx, sy);
    return wlr_surface_point_accepts_input(member->surface, *sx, *sy);
}

static void to_local(const scaled_surface *member, double *sx, double *sy)
{
    const struct wlr_surface_state *current = &member->surface->current;
    const struct wlr_scene_buffer *buffer = member->buffer;
    double scale = member->set->scale;
    *sx *= buffer->dst_width > 0 ? (double) current->width / buffer->dst_width : scale;
    *sy *= buffer->dst_height > 0 ? (double) current->height / buffer->dst_height : scale;
}

static scaled_surface *member_of(struct wlr_scene_buffer *buffer)
{
    struct wlr_addon *addon = wlr_addon_find(&buffer->node.addons, buffer, &scaled_surface_addon);
    if (addon == NULL) {
        return NULL;
    }
    scaled_surface *member = wl_container_of(addon, member, addon);
    return member;
}

static void show_buffer(scaled_surface *member)
{
    struct wlr_surface *surface = member->surface;
    const struct wlr_surface_state *current = &surface->current;
    if (surface->buffer == NULL || current->width <= 0 || current->height <= 0) {
        release_client_buffer(member->buffer);
        wlr_scene_buffer_set_buffer(member->buffer, NULL);
        return;
    }
    struct wlr_fbox source;
    wlr_surface_get_buffer_source_box(surface, &source);
    wlr_scene_buffer_set_source_box(member->buffer, &source);
    wlr_scene_buffer_set_transform(member->buffer, current->transform);
    const struct wlr_alpha_modifier_surface_v1_state *alpha =
        wlr_alpha_modifier_v1_get_surface_state(surface);
    if (alpha != NULL) {
        wlr_scene_buffer_set_opacity(member->buffer, (float) alpha->multiplier);
    }
    fit(member);
    set_buffer(member);
}

static void set_buffer(scaled_surface *member)
{
    struct wlr_client_buffer *client_buffer = member->surface->buffer;
    release_client_buffer(member->buffer);
    bool single_pixel = client_buffer->source != NULL
                        && wlr_single_pixel_buffer_v1_try_from_buffer(client_buffer->source);
    if (!single_pixel) {
        client_buffer->WLR_PRIVATE.n_ignore_locks++;
    }
    struct wlr_scene_buffer_set_buffer_options options = {
        .damage = &member->surface->buffer_damage,
    };
    struct wlr_linux_drm_syncobj_surface_v1_state *sync =
        wlr_linux_drm_syncobj_v1_get_surface_state(member->surface);
    if (sync != NULL) {
        options.wait_timeline = sync->acquire_timeline;
        options.wait_point = sync->acquire_point;
    }
    wlr_scene_buffer_set_buffer_with_options(member->buffer, &client_buffer->base, &options);
}

static void schedule_frame(scaled_surface *member)
{
    int x = 0;
    int y = 0;
    bool enabled = wlr_scene_node_coords(&member->buffer->node, &x, &y);
    struct wlr_output *output = frame_pacing_output(member->surface);
    bool waiting = !wl_list_empty(&member->surface->current.frame_callback_list);
    if (enabled && output != NULL && waiting) {
        wlr_output_schedule_frame(output);
    }
}

static struct wlr_output *frame_pacing_output(struct wlr_surface *surface)
{
    struct wlr_output *fastest = NULL;
    struct wlr_surface_output *entered;
    wl_list_for_each(entered, &surface->current_outputs, link) {
        bool faster = fastest == NULL || entered->output->refresh > fastest->refresh;
        if (!entered->WLR_PRIVATE.suspended && faster) {
            fastest = entered->output;
        }
    }
    return fastest;
}

static void fit(scaled_surface *member)
{
    const struct wlr_surface_state *current = &member->surface->current;
    if (current->width <= 0 || current->height <= 0) {
        return;
    }
    fit_size(member);
    fit_opaque_region(member, member->set->scale);
}

static void fit_size(scaled_surface *member)
{
    const struct wlr_surface_state *current = &member->surface->current;
    if (current->width <= 0 || current->height <= 0) {
        return;
    }
    const scaled_surface_sizer *sizer = &member->set->sizer;
    wm_size size = sizer->size(sizer->data, member->surface,
                               (wm_size){current->width, current->height});
    wlr_scene_buffer_set_dest_size(member->buffer, size.width, size.height);
}

static void fit_opaque_region(scaled_surface *member, double scale)
{
    pixman_region32_t opaque;
    pixman_region32_init(&opaque);
    int count = 0;
    const pixman_box32_t *boxes = pixman_region32_rectangles(&member->surface->opaque_region,
                                                              &count);
    for (int i = 0; i < count; i++) {
        int x1 = (int) ceil(boxes[i].x1 / scale);
        int y1 = (int) ceil(boxes[i].y1 / scale);
        int x2 = (int) floor(boxes[i].x2 / scale);
        int y2 = (int) floor(boxes[i].y2 / scale);
        if (x2 > x1 && y2 > y1) {
            pixman_region32_union_rect(&opaque, &opaque, x1, y1, (unsigned) (x2 - x1),
                                       (unsigned) (y2 - y1));
        }
    }
    wlr_scene_buffer_set_opaque_region(member->buffer, &opaque);
    pixman_region32_fini(&opaque);
}

static void release_client_buffer(struct wlr_scene_buffer *buffer)
{
    struct wlr_client_buffer *client_buffer =
        buffer->buffer != NULL ? wlr_client_buffer_get(buffer->buffer) : NULL;
    if (client_buffer != NULL && client_buffer->WLR_PRIVATE.n_ignore_locks > 0) {
        client_buffer->WLR_PRIVATE.n_ignore_locks--;
    }
}

static void forget(scaled_surface *member)
{
    listener_disconnect(&member->commit);
    wl_list_remove(&member->link);
    free(member);
}
