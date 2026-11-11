#include "window_capture_source.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <wlr/backend/interface.h>
#include <wlr/interfaces/wlr_ext_image_capture_source_v1.h>
#include <wlr/interfaces/wlr_output.h>
#include <wlr/types/wlr_ext_image_copy_capture_v1.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/util/log.h>

#include "util/listener.h"
#include "output/output.h"
#include "render/scene_buffers.h"
#include "xwayland/xwayland.h"

struct window_capture_source {
    struct wlr_ext_image_capture_source_v1 base;
    compositor *server;
    view *view;
    struct wlr_backend backend;
    struct wlr_output output;
    struct wlr_scene *scene;
    struct wlr_scene_tree *content;
    struct wlr_scene_tree *surface;
    struct wlr_scene_output *scene_output;
    bool constrained;
    size_t started;
    struct wl_listener output_frame;
    struct wl_listener surface_destroy;
    struct wl_listener view_changed;
};

typedef struct capture_frame_event {
    struct wlr_ext_image_capture_source_v1_frame_event base;
    struct wlr_buffer *buffer;
    struct timespec when;
} capture_frame_event;

typedef struct surface_place {
    struct wlr_surface *surface;
    int x, y;
    struct wlr_output *output;
    bool found;
} surface_place;

typedef struct capture_mode {
    float scale;
    wm_size size;
} capture_mode;

static void source_start(struct wlr_ext_image_capture_source_v1 *base, bool with_cursors);
static void source_stop(struct wlr_ext_image_capture_source_v1 *base);
static void source_request_frame(struct wlr_ext_image_capture_source_v1 *base, bool schedule);
static void source_copy_frame(struct wlr_ext_image_capture_source_v1 *base,
                              struct wlr_ext_image_copy_capture_frame_v1 *frame,
                              struct wlr_ext_image_capture_source_v1_frame_event *base_event);
static bool output_test(struct wlr_output *output, const struct wlr_output_state *state);
static bool output_commit(struct wlr_output *output, const struct wlr_output_state *state);
static bool apply_mode(window_capture_source *source, const struct wlr_output_state *state);
static void emit_frame(window_capture_source *source, const struct wlr_output_state *state);
static void handle_output_frame(struct wl_listener *listener, void *data);
static void handle_surface_destroy(struct wl_listener *listener, void *data);
static void handle_view_changed(struct wl_listener *listener, void *data);
static void init_output(window_capture_source *source, size_t serial);
static bool build_scene(window_capture_source *source);
static void show_frame(window_capture_source *source);
static void hide_frame(window_capture_source *source);
static void render(window_capture_source *source);
static capture_mode mode_for(window_capture_source *source, const wm_rect *frame);
static void stage_mode(const struct wlr_output *output, struct wlr_output_state *state,
                       const capture_mode *mode);
static float sync_scene(window_capture_source *source);
static void find_surface(struct wlr_scene_buffer *buffer, int sx, int sy, void *data);

static const struct wlr_ext_image_capture_source_v1_interface source_impl = {
    .start = source_start,
    .stop = source_stop,
    .request_frame = source_request_frame,
    .copy_frame = source_copy_frame,
};

static const struct wlr_backend_impl backend_impl = {0};

static const struct wlr_output_impl output_impl = {
    .test = output_test,
    .commit = output_commit,
};

window_capture_source *window_capture_source_create(compositor *server, view *v, size_t serial)
{
    if (view_surface(v) == NULL) {
        return NULL;
    }
    window_capture_source *s = calloc(1, sizeof(*s));
    if (s == NULL) {
        return NULL;
    }
    s->server = server;
    s->view = v;
    init_output(s, serial);
    if (!wlr_output_init_render(&s->output, server->allocator, server->renderer) ||
        !build_scene(s)) {
        wlr_log(WLR_ERROR, "cannot create the capture source of a window");
        window_capture_source_destroy(s);
        return NULL;
    }
    listener_connect(&s->output_frame, &s->output.events.frame, handle_output_frame);
    listener_connect(&s->surface_destroy, &s->surface->node.events.destroy,
                     handle_surface_destroy);
    return s;
}

void window_capture_source_destroy(window_capture_source *s)
{
    if (s->started > 0) {
        hide_frame(s);
    }
    listener_disconnect(&s->output_frame);
    listener_disconnect(&s->surface_destroy);
    listener_disconnect(&s->view_changed);
    wlr_ext_image_capture_source_v1_finish(&s->base);
    if (s->scene_output != NULL) {
        wlr_scene_output_destroy(s->scene_output);
    }
    if (s->scene != NULL) {
        wlr_scene_node_destroy(&s->scene->tree.node);
    }
    wlr_output_finish(&s->output);
    wlr_backend_finish(&s->backend);
    free(s);
}

struct wlr_ext_image_capture_source_v1 *window_capture_source_base(window_capture_source *s)
{
    return &s->base;
}

view *window_capture_source_view(const window_capture_source *s)
{
    return s->view;
}

static void init_output(window_capture_source *s, size_t serial)
{
    wlr_ext_image_capture_source_v1_init(&s->base, &source_impl);
    wlr_backend_init(&s->backend, &backend_impl);
    s->backend.buffer_caps = WLR_BUFFER_CAP_DMABUF | WLR_BUFFER_CAP_SHM;
    wlr_output_init(&s->output, &s->backend, &output_impl, s->server->loop, NULL);
    char name[32];
    snprintf(name, sizeof(name), "WINDOW-CAPTURE-%zu", serial);
    wlr_output_set_name(&s->output, name);
    wl_list_init(&s->output_frame.link);
    wl_list_init(&s->surface_destroy.link);
    wl_list_init(&s->view_changed.link);
}

static bool build_scene(window_capture_source *s)
{
    s->scene = wlr_scene_create();
    if (s->scene == NULL) {
        return false;
    }
    s->content = wlr_scene_tree_create(&s->scene->tree);
    if (s->content == NULL) {
        return false;
    }
    s->surface = xwayland_surface_tree(s->server, s->content, view_surface(s->view));
    if (s->surface == NULL) {
        return false;
    }
    s->scene_output = wlr_scene_output_create(s->scene, &s->output);
    if (s->scene_output == NULL) {
        return false;
    }
    wlr_scene_output_set_position(s->scene_output, 0, 0);
    return true;
}

static void show_frame(window_capture_source *s)
{
    struct wlr_scene_tree *frame = view_mirror_frame(s->view, &s->scene->tree);
    if (frame != NULL) {
        wlr_scene_node_lower_to_bottom(&frame->node);
    }
}

static void hide_frame(window_capture_source *s)
{
    view_mirror_frame(s->view, NULL);
}

static void source_start(struct wlr_ext_image_capture_source_v1 *base, bool with_cursors)
{
    (void) with_cursors;
    window_capture_source *s = wl_container_of(base, s, base);
    if (s->started++ > 0) {
        return;
    }
    show_frame(s);
    listener_connect(&s->view_changed, &s->server->view_events.changed, handle_view_changed);
    render(s);
    scene_output_send_frame_done_now(s->scene_output);
}

static void source_stop(struct wlr_ext_image_capture_source_v1 *base)
{
    window_capture_source *s = wl_container_of(base, s, base);
    if (s->started == 0 || --s->started > 0) {
        return;
    }
    hide_frame(s);
    listener_disconnect(&s->view_changed);
    struct wlr_output_state state;
    wlr_output_state_init(&state);
    wlr_output_state_set_enabled(&state, false);
    wlr_output_commit_state(&s->output, &state);
    wlr_output_state_finish(&state);
}

static void source_request_frame(struct wlr_ext_image_capture_source_v1 *base, bool schedule)
{
    window_capture_source *s = wl_container_of(base, s, base);
    if (s->output.frame_pending) {
        wlr_output_send_frame(&s->output);
    }
    if (schedule) {
        wlr_output_update_needs_frame(&s->output);
    }
}

static void source_copy_frame(struct wlr_ext_image_capture_source_v1 *base,
                              struct wlr_ext_image_copy_capture_frame_v1 *frame,
                              struct wlr_ext_image_capture_source_v1_frame_event *base_event)
{
    window_capture_source *s = wl_container_of(base, s, base);
    capture_frame_event *event = wl_container_of(base_event, event, base);
    if (wlr_ext_image_copy_capture_frame_v1_copy_buffer(frame, event->buffer,
                                                        s->output.renderer)) {
        wlr_ext_image_copy_capture_frame_v1_ready(frame, s->output.transform, &event->when);
    }
}

static void render(window_capture_source *s)
{
    wm_rect frame = view_shown_frame(s->view);
    if (frame.width <= 0 || frame.height <= 0) {
        return;
    }
    capture_mode mode = mode_for(s, &frame);
    struct wlr_output_state state;
    wlr_output_state_init(&state);
    stage_mode(&s->output, &state, &mode);
    if (!wlr_scene_output_build_state(s->scene_output, &state, NULL) ||
        !wlr_output_commit_state(&s->output, &state)) {
        wlr_log(WLR_DEBUG, "cannot render the capture of a window");
    }
    wlr_output_state_finish(&state);
}

static capture_mode mode_for(window_capture_source *s, const wm_rect *frame)
{
    float scale = sync_scene(s);
    wm_size size = {(int) ceilf((float) frame->width * scale),
                    (int) ceilf((float) frame->height * scale)};
    return (capture_mode){scale, size};
}

static void stage_mode(const struct wlr_output *output, struct wlr_output_state *state,
                       const capture_mode *mode)
{
    if (!output->enabled) {
        wlr_output_state_set_enabled(state, true);
    }
    if (output->scale != mode->scale) {
        wlr_output_state_set_scale(state, mode->scale);
    }
    if (output->width != mode->size.width || output->height != mode->size.height) {
        wlr_output_state_set_custom_mode(state, mode->size.width, mode->size.height, 0);
    }
}

static float sync_scene(window_capture_source *s)
{
    struct wlr_scene_node *content = &view_content_tree(s->view)->node;
    surface_place place = {.surface = view_surface(s->view)};
    wlr_scene_node_for_each_buffer(content, find_surface, &place);
    wlr_scene_node_set_position(&s->content->node, content->x, content->y);
    wlr_scene_node_set_enabled(&s->content->node, content->enabled);
    if (s->surface != NULL) {
        wlr_scene_node_set_position(&s->surface->node, place.x - content->x,
                                    place.y - content->y);
    }
    return place.output != NULL ? place.output->scale
                                : (float) outputs_max_scale(s->server->outputs);
}

static void find_surface(struct wlr_scene_buffer *buffer, int sx, int sy, void *data)
{
    surface_place *place = data;
    struct wlr_scene_surface *scene_surface = wlr_scene_surface_try_from_buffer(buffer);
    if (place->found || scene_surface == NULL || scene_surface->surface != place->surface) {
        return;
    }
    place->found = true;
    place->x = sx;
    place->y = sy;
    if (buffer->primary_output != NULL) {
        place->output = buffer->primary_output->output;
    }
}

static void handle_surface_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    window_capture_source *s = wl_container_of(listener, s, surface_destroy);
    listener_disconnect(&s->surface_destroy);
    s->surface = NULL;
}

static void handle_view_changed(struct wl_listener *listener, void *data)
{
    window_capture_source *s = wl_container_of(listener, s, view_changed);
    if (data != s->view) {
        return;
    }
    if (!s->constrained) {
        render(s);
        scene_output_send_frame_done_now(s->scene_output);
        return;
    }
    sync_scene(s);
}

static void handle_output_frame(struct wl_listener *listener, void *data)
{
    (void) data;
    window_capture_source *s = wl_container_of(listener, s, output_frame);
    if (wlr_scene_output_needs_frame(s->scene_output)) {
        render(s);
    }
    scene_output_send_frame_done_now(s->scene_output);
}

static bool output_test(struct wlr_output *output, const struct wlr_output_state *state)
{
    (void) output;
    uint32_t supported = WLR_OUTPUT_STATE_BACKEND_OPTIONAL | WLR_OUTPUT_STATE_BUFFER |
                         WLR_OUTPUT_STATE_ENABLED | WLR_OUTPUT_STATE_MODE |
                         WLR_OUTPUT_STATE_SCALE | WLR_OUTPUT_STATE_DAMAGE;
    return (state->committed & ~supported) == 0;
}

static bool output_commit(struct wlr_output *output, const struct wlr_output_state *state)
{
    window_capture_source *s = wl_container_of(output, s, output);
    if ((state->committed & WLR_OUTPUT_STATE_ENABLED) && !state->enabled) {
        return true;
    }
    if (!apply_mode(s, state) || !(state->committed & WLR_OUTPUT_STATE_BUFFER)) {
        return false;
    }
    emit_frame(s, state);
    return true;
}

static bool apply_mode(window_capture_source *s, const struct wlr_output_state *state)
{
    struct wlr_output *output = &s->output;
    if (!(state->committed & WLR_OUTPUT_STATE_MODE)) {
        return true;
    }
    if (!wlr_output_configure_primary_swapchain(output, state, &output->swapchain)) {
        return false;
    }
    s->constrained = wlr_ext_image_capture_source_v1_set_constraints_from_swapchain(
        &s->base, output->swapchain, output->renderer);
    return true;
}

static void emit_frame(window_capture_source *s, const struct wlr_output_state *state)
{
    struct wlr_buffer *buffer = state->buffer;
    pixman_region32_t full;
    pixman_region32_init_rect(&full, 0, 0, buffer->width, buffer->height);
    capture_frame_event event = {
        .base = {.damage = (state->committed & WLR_OUTPUT_STATE_DAMAGE) ? &state->damage : &full},
        .buffer = buffer,
    };
    clock_gettime(CLOCK_MONOTONIC, &event.when);
    wl_signal_emit_mutable(&s->base.events.frame, &event.base);
    pixman_region32_fini(&full);
}
