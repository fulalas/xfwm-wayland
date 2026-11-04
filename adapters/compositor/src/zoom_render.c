#include <pixman.h>
#include <wlr/render/allocator.h>
#include <wlr/render/pass.h>
#include <wlr/render/swapchain.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/render/wlr_texture.h>
#include <wlr/types/wlr_buffer.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/util/log.h>

#include "output_private.h"
#include "zoom.h"
#include "zoom_math.h"
#include "zoom_private.h"

static void sync_cursor_lock(output *output, bool locked);
static bool render_frame(const zoom *zoom, output *output);
static bool ensure_swapchain(output *output);
static bool primary_format_ready(output *output);
static void keep_source(output *output, const struct wlr_output_state *state);
static bool compose(const zoom *zoom, output *output, struct wlr_output_state *state);
static bool draw_zoomed(const zoom *zoom, output *output, struct wlr_buffer *target);
static zoom_box source_box(const zoom *zoom, const output *output);
static void set_full_damage(struct wlr_output_state *state, const struct wlr_buffer *target);

void zoom_render(zoom *z, output *o)
{
    sync_cursor_lock(o, z->view.zoom_pointer);
    if (!o->zoom.view_changed && !wlr_scene_output_needs_frame(o->scene_output)) {
        return;
    }
    if (render_frame(z, o)) {
        o->zoom.view_changed = false;
    } else {
        wlr_output_schedule_frame(o->wlr_output);
    }
}

static void sync_cursor_lock(output *o, bool locked)
{
    if (o->zoom.cursor_locked == locked) {
        return;
    }
    wlr_output_lock_software_cursors(o->wlr_output, locked);
    o->zoom.cursor_locked = locked;
}

static bool render_frame(const zoom *z, output *o)
{
    if (!ensure_swapchain(o)) {
        return false;
    }
    struct wlr_output_state state;
    wlr_output_state_init(&state);
    struct wlr_scene_output_state_options options = {.swapchain = o->zoom.swapchain};
    bool ok = wlr_scene_output_build_state(o->scene_output, &state, &options);
    if (ok) {
        keep_source(o, &state);
        ok = compose(z, o, &state) && output_commit_state(o, &state);
    }
    wlr_output_state_finish(&state);
    return ok;
}

static bool ensure_swapchain(output *o)
{
    struct wlr_output *wlr_output = o->wlr_output;
    struct wlr_swapchain *current = o->zoom.swapchain;
    if (current != NULL && current->width == wlr_output->width &&
        current->height == wlr_output->height) {
        return true;
    }
    if (current != NULL) {
        wlr_swapchain_destroy(current);
        o->zoom.swapchain = NULL;
    }
    if (!primary_format_ready(o)) {
        return false;
    }
    o->zoom.swapchain = wlr_swapchain_create(o->server->allocator, wlr_output->width,
                                             wlr_output->height, &wlr_output->swapchain->format);
    if (o->zoom.swapchain == NULL) {
        wlr_log(WLR_ERROR, "cannot create zoom swapchain for %s", wlr_output->name);
    }
    return o->zoom.swapchain != NULL;
}

static bool primary_format_ready(output *o)
{
    struct wlr_output_state empty;
    wlr_output_state_init(&empty);
    bool ok = wlr_output_configure_primary_swapchain(o->wlr_output, &empty,
                                                     &o->wlr_output->swapchain);
    wlr_output_state_finish(&empty);
    return ok && o->wlr_output->swapchain != NULL;
}

static void keep_source(output *o, const struct wlr_output_state *state)
{
    if ((state->committed & WLR_OUTPUT_STATE_BUFFER) == 0) {
        return;
    }
    if (o->zoom.source != NULL) {
        wlr_buffer_unlock(o->zoom.source);
    }
    o->zoom.source = wlr_buffer_lock(state->buffer);
}

static bool compose(const zoom *z, output *o, struct wlr_output_state *state)
{
    struct wlr_output *wlr_output = o->wlr_output;
    if (o->zoom.source == NULL ||
        !wlr_output_configure_primary_swapchain(wlr_output, state, &wlr_output->swapchain)) {
        return false;
    }
    struct wlr_buffer *target = wlr_swapchain_acquire(wlr_output->swapchain);
    if (target == NULL) {
        return false;
    }
    bool ok = draw_zoomed(z, o, target);
    if (ok) {
        wlr_output_state_set_buffer(state, target);
        state->buffer_src_box = (struct wlr_fbox){0};
        state->buffer_dst_box = (struct wlr_box){0};
        set_full_damage(state, target);
    }
    wlr_buffer_unlock(target);
    return ok;
}

static bool draw_zoomed(const zoom *z, output *o, struct wlr_buffer *target)
{
    struct wlr_renderer *renderer = o->server->renderer;
    struct wlr_texture *texture = wlr_texture_from_buffer(renderer, o->zoom.source);
    if (texture == NULL) {
        return false;
    }
    struct wlr_render_pass *pass = wlr_renderer_begin_buffer_pass(renderer, target, NULL);
    if (pass == NULL) {
        wlr_texture_destroy(texture);
        return false;
    }
    zoom_box box = source_box(z, o);
    wlr_render_pass_add_texture(pass, &(struct wlr_render_texture_options){
                                          .texture = texture,
                                          .src_box = {box.x, box.y, box.width, box.height},
                                          .dst_box = {0, 0, target->width, target->height},
                                          .filter_mode = WLR_SCALE_FILTER_BILINEAR,
                                          .blend_mode = WLR_RENDER_BLEND_MODE_NONE,
                                      });
    bool ok = wlr_render_pass_submit(pass);
    wlr_texture_destroy(texture);
    return ok;
}

static zoom_box source_box(const zoom *z, const output *o)
{
    wm_rect full = output_full(o);
    double scale = o->wlr_output->scale;
    zoom_request request = {
        .size = {o->wlr_output->width, o->wlr_output->height},
        .center = {(int) ((z->view.center.x - full.x) * scale),
                   (int) ((z->view.center.y - full.y) * scale)},
        .level = z->view.level,
    };
    return zoom_source_box(&request);
}

static void set_full_damage(struct wlr_output_state *state, const struct wlr_buffer *target)
{
    pixman_region32_t damage;
    pixman_region32_init_rect(&damage, 0, 0, (unsigned) target->width,
                              (unsigned) target->height);
    wlr_output_state_set_damage(state, &damage);
    pixman_region32_fini(&damage);
}
