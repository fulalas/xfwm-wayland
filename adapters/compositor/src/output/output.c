#include <math.h>
#include <stdlib.h>
#include <wlr/backend.h>
#include <wlr/interfaces/wlr_output.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_damage_ring.h>
#include <wlr/types/wlr_fractional_scale_v1.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/util/log.h>

#include "color_preference.h"
#include "displays.h"
#include "shell/layer_shell.h"
#include "util/listener.h"
#include "output_brightness.h"
#include "output_private.h"
#include "output_repaint.h"
#include "zoom.h"

static void handle_new_output(struct wl_listener *listener, void *data);
static void handle_layout_change(struct wl_listener *listener, void *data);
static void handle_frame(struct wl_listener *listener, void *data);
static void handle_commit(struct wl_listener *listener, void *data);
static void handle_request_state(struct wl_listener *listener, void *data);
static void handle_destroy(struct wl_listener *listener, void *data);
static void handle_arrange_idle(void *data);
static output *output_create(outputs *outputs, struct wlr_output *wlr_output);
static void output_destroy(output *output);
static bool init_render(const compositor *server, struct wlr_output *wlr_output);

outputs *outputs_create(compositor *server)
{
    outputs *outs = calloc(1, sizeof(*outs));
    if (outs == NULL) {
        return NULL;
    }
    outs->server = server;
    outs->max_scale = 1.0;
    wl_list_init(&outs->list);
    outputs_renderer_changed(outs);
    listener_connect(&outs->new_output, &server->backend->events.new_output, handle_new_output);
    listener_connect(&outs->layout_change, &server->layout->events.change, handle_layout_change);
    return outs;
}

void outputs_destroy(outputs *outs)
{
    output *o;
    output *tmp;
    wl_list_for_each_safe(o, tmp, &outs->list, link) {
        output_destroy(o);
    }
    listener_disconnect(&outs->new_output);
    listener_disconnect(&outs->layout_change);
    if (outs->arrange_idle != NULL) {
        wl_event_source_remove(outs->arrange_idle);
    }
    free(outs);
}

output *outputs_at_point(const outputs *outs, wm_point point)
{
    struct wlr_output *wlr_output =
        wlr_output_layout_output_at(outs->server->layout, point.x, point.y);
    return wlr_output != NULL ? output_from_wlr(wlr_output) : NULL;
}

output *outputs_near_point(const outputs *outs, wm_point point)
{
    output *found = outputs_at_point(outs, point);
    return found != NULL ? found : outputs_at_index(outs, 0);
}

output *outputs_for_rect(const outputs *outs, const wm_rect *rect)
{
    return outputs_near_point(outs, wm_rect_center(rect));
}

double outputs_max_scale(const outputs *outs)
{
    return outs->max_scale;
}

void outputs_schedule_arrange(outputs *outs)
{
    if (outs->arrange_idle == NULL) {
        outs->arrange_idle = wl_event_loop_add_idle(outs->server->loop, handle_arrange_idle, outs);
    }
}

void outputs_mark_changed(outputs *outs)
{
    outs->changed = true;
    outputs_schedule_arrange(outs);
}

void outputs_schedule_frames(outputs *outs)
{
    output *o;
    wl_list_for_each(o, &outs->list, link) {
        wlr_output_schedule_frame(o->wlr_output);
    }
}

bool outputs_renderer_changed(outputs *outs)
{
    compositor *server = outs->server;
    outs->can_time_renders = output_repaint_renderer_can_time(server->renderer);
    bool all_ok = true;
    output *o;
    wl_list_for_each(o, &outs->list, link) {
        zoom_release(o);
        output_repaint_reset_timing(o);
        all_ok = init_render(server, o->wlr_output) && all_ok;
    }
    outputs_schedule_frames(outs);
    return all_ok;
}

bool outputs_rebuild_swapchains(outputs *outs)
{
    for (output *o = outputs_next_active(outs, NULL); o != NULL; o = outputs_next_active(outs, o)) {
        if (o->wlr_output->enabled && !output_rebuild_swapchain(o)) {
            return false;
        }
    }
    return true;
}

bool output_rebuild_swapchain(output *o)
{
    struct wlr_output *w = o->wlr_output;
    struct wlr_output_state empty;
    wlr_output_state_init(&empty);
    bool ok = wlr_output_configure_primary_swapchain(w, &empty, &w->swapchain);
    wlr_output_state_finish(&empty);
    return ok && w->swapchain != NULL;
}

wm_rect output_full(const output *o)
{
    struct wlr_box box = {0};
    wlr_output_layout_get_box(o->server->layout, o->wlr_output, &box);
    return (wm_rect){box.x, box.y, box.width, box.height};
}

wm_rect output_usable(const output *o)
{
    return o->usable;
}

struct wlr_output *output_wlr(const output *o)
{
    return o->wlr_output;
}

void output_announce_scale(const output *o, struct wlr_surface *surface)
{
    float scale = o->wlr_output->scale;
    wlr_fractional_scale_v1_notify_scale(surface, scale);
    wlr_surface_set_preferred_buffer_scale(surface, (int32_t) ceilf(scale));
}

output *output_from_wlr(struct wlr_output *wlr_output)
{
    return wlr_output->data;
}

static void handle_new_output(struct wl_listener *listener, void *data)
{
    outputs *outs = wl_container_of(listener, outs, new_output);
    compositor *server = outs->server;
    struct wlr_output *wlr_output = data;
    if (wlr_output->non_desktop) {
        return;
    }
    if (!init_render(server, wlr_output)) {
        return;
    }
    output *o = output_create(outs, wlr_output);
    if (o == NULL) {
        wlr_log(WLR_ERROR, "cannot create output %s", wlr_output->name);
        return;
    }
    displays_output_added(server->displays, o);
}

static void handle_layout_change(struct wl_listener *listener, void *data)
{
    (void) data;
    outputs *outs = wl_container_of(listener, outs, layout_change);
    outputs_mark_changed(outs);
}

static void handle_frame(struct wl_listener *listener, void *data)
{
    (void) data;
    output *o = wl_container_of(listener, o, frame);
    output_repaint_frame(o);
}

static void handle_commit(struct wl_listener *listener, void *data)
{
    output *o = wl_container_of(listener, o, commit);
    const struct wlr_output_event_commit *event = data;
    output_brightness_committed(o, event->state);
    bool color_changed = (event->state->committed & WLR_OUTPUT_STATE_IMAGE_DESCRIPTION) != 0;
    if (!color_changed) {
        return;
    }
    if (o->scene_output != NULL) {
        wlr_damage_ring_add_whole(&o->scene_output->damage_ring);
        wlr_output_update_needs_frame(o->wlr_output);
    }
    color_preference_refresh(o->server);
}

static void handle_request_state(struct wl_listener *listener, void *data)
{
    output *o = wl_container_of(listener, o, request_state);
    const struct wlr_output_event_request_state *event = data;
    if (!wlr_output_commit_state(o->wlr_output, event->state)) {
        wlr_log(WLR_ERROR, "cannot apply requested state on output %s", o->wlr_output->name);
    }
    outputs_mark_changed(o->server->outputs);
}

static void handle_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    output *o = wl_container_of(listener, o, destroy);
    compositor *server = o->server;
    output_destroy(o);
    outputs_mark_changed(server->outputs);
    displays_output_removed(server->displays);
}

static void handle_arrange_idle(void *data)
{
    outputs *outs = data;
    outs->arrange_idle = NULL;
    outputs_arrange(outs);
}

static output *output_create(outputs *outs, struct wlr_output *wlr_output)
{
    output *o = calloc(1, sizeof(*o));
    if (o == NULL) {
        return NULL;
    }
    o->server = outs->server;
    o->wlr_output = wlr_output;
    o->stats = paint_stats_init();
    output_brightness_init(o);
    if (!output_repaint_init(o)) {
        free(o);
        return NULL;
    }
    wlr_output->data = o;
    wl_list_init(&o->scene_destroy.link);
    listener_connect(&o->frame, &wlr_output->events.frame, handle_frame);
    listener_connect(&o->commit, &wlr_output->events.commit, handle_commit);
    listener_connect(&o->request_state, &wlr_output->events.request_state, handle_request_state);
    listener_connect(&o->destroy, &wlr_output->events.destroy, handle_destroy);
    wl_list_insert(outs->list.prev, &o->link);
    return o;
}

static void output_destroy(output *o)
{
    zoom_release(o);
    output_repaint_finish(o);
    output_brightness_release(o);
    if (o->server->layer_shell != NULL) {
        layer_shell_output_destroyed(o->server->layer_shell, o);
    }
    listener_disconnect(&o->frame);
    listener_disconnect(&o->commit);
    listener_disconnect(&o->request_state);
    listener_disconnect(&o->destroy);
    listener_disconnect(&o->scene_destroy);
    o->wlr_output->data = NULL;
    edid_release(&o->display.edid);
    wl_list_remove(&o->link);
    free(o);
}

static bool init_render(const compositor *server, struct wlr_output *wlr_output)
{
    if (wlr_output_init_render(wlr_output, server->allocator, server->renderer)) {
        return true;
    }
    wlr_log(WLR_ERROR, "cannot init rendering on output %s", wlr_output->name);
    return false;
}
