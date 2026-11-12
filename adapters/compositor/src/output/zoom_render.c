#include <pixman.h>
#include <wlr/render/allocator.h>
#include <wlr/render/drm_syncobj.h>
#include <wlr/render/pass.h>
#include <wlr/render/swapchain.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/render/wlr_texture.h>
#include <wlr/types/wlr_buffer.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/util/box.h>
#include <wlr/util/log.h>
#include <wlr/util/transform.h>

#include "output_brightness.h"
#include "output_private.h"
#include "output_repaint.h"
#include "render/render_sync.h"
#include "zoom.h"
#include "zoom_math.h"
#include "zoom_private.h"

enum {
    RENDER_RETRY_MAX = 3,
};

typedef struct zoom_pass {
    struct wlr_buffer *target;
    struct wlr_drm_syncobj_timeline *wait_timeline;
    uint64_t wait_point;
} zoom_pass;

static void sync_cursor_lock(output *output, bool locked);
static bool render_frame(const zoom *zoom, output *output);
static bool ensure_swapchain(output *output);
static bool swapchain_fits(const output *output);
static void drop_swapchain(output *output);
static void keep_source(output *output, const struct wlr_output_state *state);
static bool compose(const zoom *zoom, output *output, struct wlr_output_state *state);
static void attach_target(output *output, struct wlr_output_state *state,
                          struct wlr_buffer *target);
static bool draw_zoomed(const zoom *zoom, output *output, const zoom_pass *pass);
static struct wlr_render_texture_options texture_options(const zoom *zoom, const output *output,
                                                         const zoom_pass *pass);
static struct wlr_buffer_pass_options signal_options(const output *output);
static void hand_fence_to_output(const output *output, struct wlr_output_state *state);
static zoom_box source_box(const zoom *zoom, const output *output);
static wm_point buffer_center(const zoom *z, const output *o);
static void set_full_damage(struct wlr_output_state *state, const struct wlr_buffer *target);

void zoom_render(zoom *z, output *o)
{
    sync_cursor_lock(o, z->view.zoom_pointer);
    if (!o->zoom.view_changed && !wlr_scene_output_needs_frame(o->scene_output)) {
        return;
    }
    if (render_frame(z, o)) {
        o->zoom.view_changed = false;
        o->zoom.failures = 0;
    } else if (++o->zoom.failures < RENDER_RETRY_MAX) {
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
    struct wlr_scene_output_state_options options = {
        .swapchain = o->zoom.swapchain,
    };
    output_tell_scene_color(o, &state);
    output_brightness_frame(o, &state);
    bool ok = output_repaint_build_state(o, &state, &options);
    if (ok) {
        keep_source(o, &state);
        ok = compose(z, o, &state) && output_commit_state(o, &state);
    }
    wlr_output_state_finish(&state);
    return ok;
}

static bool ensure_swapchain(output *o)
{
    if (swapchain_fits(o)) {
        return true;
    }
    drop_swapchain(o);
    if (!output_rebuild_swapchain(o)) {
        return false;
    }
    struct wlr_output *wlr_output = o->wlr_output;
    o->zoom.swapchain = wlr_swapchain_create(o->server->allocator, wlr_output->width,
                                             wlr_output->height, &wlr_output->swapchain->format);
    if (o->zoom.swapchain == NULL) {
        wlr_log(WLR_ERROR, "cannot create zoom swapchain for %s", wlr_output->name);
    } else if (o->zoom.timeline == NULL) {
        o->zoom.timeline = render_sync_timeline_create(o->server);
    }
    return o->zoom.swapchain != NULL;
}

static bool swapchain_fits(const output *o)
{
    const struct wlr_output *wlr_output = o->wlr_output;
    const struct wlr_swapchain *current = o->zoom.swapchain;
    return current != NULL && current->width == wlr_output->width &&
           current->height == wlr_output->height &&
           current->format.format == wlr_output->render_format;
}

static void drop_swapchain(output *o)
{
    if (o->zoom.swapchain != NULL) {
        wlr_swapchain_destroy(o->zoom.swapchain);
        o->zoom.swapchain = NULL;
    }
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
    zoom_pass pass = {target, state->wait_timeline, state->wait_point};
    o->zoom.point++;
    bool ok = draw_zoomed(z, o, &pass);
    if (ok) {
        attach_target(o, state, target);
    }
    wlr_buffer_unlock(target);
    return ok;
}

static void attach_target(output *o, struct wlr_output_state *state, struct wlr_buffer *target)
{
    wlr_output_state_set_buffer(state, target);
    state->buffer_src_box = (struct wlr_fbox){0};
    state->buffer_dst_box = (struct wlr_box){0};
    set_full_damage(state, target);
    hand_fence_to_output(o, state);
}

static bool draw_zoomed(const zoom *z, output *o, const zoom_pass *zp)
{
    struct wlr_renderer *renderer = o->server->renderer;
    struct wlr_texture *texture = wlr_texture_from_buffer(renderer, o->zoom.source);
    if (texture == NULL) {
        return false;
    }
    struct wlr_buffer_pass_options options = signal_options(o);
    struct wlr_render_pass *pass = wlr_renderer_begin_buffer_pass(renderer, zp->target, &options);
    if (pass == NULL) {
        wlr_texture_destroy(texture);
        return false;
    }
    struct wlr_render_texture_options draw = texture_options(z, o, zp);
    draw.texture = texture;
    wlr_render_pass_add_texture(pass, &draw);
    bool ok = wlr_render_pass_submit(pass);
    wlr_texture_destroy(texture);
    return ok;
}

static struct wlr_render_texture_options texture_options(const zoom *z, const output *o,
                                                         const zoom_pass *zp)
{
    zoom_box box = source_box(z, o);
    return (struct wlr_render_texture_options){
        .src_box = {box.x, box.y, box.width, box.height},
        .dst_box = {0, 0, zp->target->width, zp->target->height},
        .filter_mode = zoom_smooths(z->view.level) ? WLR_SCALE_FILTER_BILINEAR
                                                   : WLR_SCALE_FILTER_NEAREST,
        .blend_mode = WLR_RENDER_BLEND_MODE_NONE,
        .wait_timeline = zp->wait_timeline,
        .wait_point = zp->wait_point,
    };
}

static struct wlr_buffer_pass_options signal_options(const output *o)
{
    if (o->zoom.timeline == NULL) {
        return (struct wlr_buffer_pass_options){0};
    }
    return (struct wlr_buffer_pass_options){
        .signal_timeline = o->zoom.timeline,
        .signal_point = o->zoom.point,
    };
}

static void hand_fence_to_output(const output *o, struct wlr_output_state *state)
{
    if (o->zoom.timeline != NULL) {
        wlr_output_state_set_wait_timeline(state, o->zoom.timeline, o->zoom.point);
    } else if (state->wait_timeline != NULL) {
        render_sync_drop_wait(state);
    }
}

static zoom_box source_box(const zoom *z, const output *o)
{
    zoom_request request = {
        .size = {o->wlr_output->width, o->wlr_output->height},
        .center = buffer_center(z, o),
        .level = z->view.level,
    };
    return zoom_source_box(&request);
}

static wm_point buffer_center(const zoom *z, const output *o)
{
    struct wlr_output *wlr_output = o->wlr_output;
    wm_rect full = output_full(o);
    double scale = wlr_output->scale;
    int width = 0;
    int height = 0;
    wlr_output_transformed_resolution(wlr_output, &width, &height);
    struct wlr_box center = {(int) ((z->view.center.x - full.x) * scale),
                             (int) ((z->view.center.y - full.y) * scale), 1, 1};
    wlr_box_transform(&center, &center, wlr_output_transform_invert(wlr_output->transform),
                      width, height);
    return (wm_point){center.x, center.y};
}

static void set_full_damage(struct wlr_output_state *state, const struct wlr_buffer *target)
{
    pixman_region32_t damage;
    pixman_region32_init_rect(&damage, 0, 0, (unsigned) target->width,
                              (unsigned) target->height);
    wlr_output_state_set_damage(state, &damage);
    pixman_region32_fini(&damage);
}
