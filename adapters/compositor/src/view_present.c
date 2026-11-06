#include <math.h>
#include <wlr/types/wlr_scene.h>

#include "output.h"
#include "scene_buffers.h"
#include "scene_layers.h"
#include "shadow_cache.h"
#include "utf8_copy.h"
#include "view_private.h"

static const double SHADOW_TEMPLATE_OPACITY = 1.0;

static void store_target(view *view, const wm_window_view *state);
static void apply_layer(view *view, wm_layer layer);
static bool geometry_ready(const view *view);
static void apply_decorations(view *view);
static void apply_shadow(view *view);
static void apply_opacity(view *view, const wm_opacity_view *previous);

void view_present(view *v, const wm_window_view *state)
{
    wm_opacity_view previous = v->target.state.opacity;
    store_target(v, state);
    apply_layer(v, state->layer);
    wlr_scene_node_set_enabled(&v->tree->node, state->visible);
    wlr_scene_node_set_enabled(&v->content->node, !state->shaded);
    wlr_scene_node_set_enabled(&v->popups->node, !state->shaded);
    apply_opacity(v, &previous);
    v->impl->send_states(v->self, &v->target.state);
    if (geometry_ready(v)) {
        view_apply_geometry(v);
    }
    wl_signal_emit(&v->server->view_events.changed, v);
}

void view_apply_geometry(view *v)
{
    const wm_window_view *s = &v->target.state;
    int content_x = s->shown_client.x - s->frame.x;
    int content_y = s->shown_client.y - s->frame.y;
    wlr_scene_node_set_position(&v->tree->node, s->frame.x, s->frame.y);
    wlr_scene_node_set_position(&v->content->node, content_x, content_y);
    wlr_scene_node_set_position(&v->popups->node, content_x, content_y);
    apply_decorations(v);
    apply_shadow(v);
    v->shown_frame = s->frame;
    v->presented = true;
}

struct wlr_scene_tree *view_mirror_frame(view *v, struct wlr_scene_tree *parent)
{
    struct wlr_scene_tree *mirror = decorations_mirror(v->decor, parent);
    if (mirror != NULL) {
        v->paint_mark.painted = false;
        if (v->presented) {
            apply_decorations(v);
        }
    }
    return mirror;
}

void view_apply_content_opacity(view *v)
{
    scene_buffers_set_opacity(v->content, (float) v->target.state.opacity.content);
}

static void store_target(view *v, const wm_window_view *state)
{
    view_target *target = &v->target;
    if (!v->presented || target->state.decor_serial != state->decor_serial) {
        target->decor = *state->decor;
    }
    target->state = *state;
    target->state.decor = &target->decor;
    utf8_copy(target->title, sizeof(target->title), state->title);
    target->state.title = target->title;
    utf8_copy(target->app_id, sizeof(target->app_id), state->app_id);
    target->state.app_id = target->app_id;
}

static void apply_layer(view *v, wm_layer layer)
{
    scene_layer wanted = layer_mapping_from_wm(layer);
    if (wanted == v->layer) {
        return;
    }
    v->layer = wanted;
    wlr_scene_node_reparent(&v->tree->node, scene_layers_tree(v->server->layers, wanted));
}

static bool geometry_ready(const view *v)
{
    const wm_window_view *s = &v->target.state;
    if (!v->presented || !s->visible || !v->handshake.awaiting) {
        return true;
    }
    wm_size target = {s->shown_client.width, s->shown_client.height};
    return wm_size_equals(target, v->handshake.committed);
}

static void apply_decorations(view *v)
{
    const wm_window_view *s = &v->target.state;
    if (!v->target.decor.has_frame) {
        decorations_hide(v->decor);
        v->paint_mark.painted = false;
        return;
    }
    double scale = outputs_max_scale(v->server->outputs);
    view_paint_mark *mark = &v->paint_mark;
    if (mark->painted && mark->serial == s->decor_serial && mark->scale == scale) {
        return;
    }
    decor_paint paint = {
        .layout = &v->target.decor,
        .title = v->target.title,
        .icon = view_icon_for(v, &v->target.decor.app_icon, scale),
        .scale = scale,
    };
    decorations_paint(v->decor, &paint);
    *mark = (view_paint_mark){true, s->decor_serial, scale};
}

static void apply_shadow(view *v)
{
    const wm_window_view *s = &v->target.state;
    if (!s->has_shadow) {
        shadow_hide(v->shadow);
        return;
    }
    shadow_image image = shadow_cache_get(v->server->shadows, SHADOW_TEMPLATE_OPACITY);
    wm_rect relative = {s->shadow.x - s->frame.x, s->shadow.y - s->frame.y, s->shadow.width,
                        s->shadow.height};
    shadow_show(v->shadow, &image, &relative);
}

static void apply_opacity(view *v, const wm_opacity_view *previous)
{
    const wm_opacity_view *opacity = &v->target.state.opacity;
    if (opacity->content != previous->content) {
        view_apply_content_opacity(v);
    }
    if (opacity->frame != previous->frame) {
        decorations_set_opacity(v->decor, (float) opacity->frame);
    }
    if (opacity->shadow != previous->shadow) {
        shadow_set_opacity(v->shadow, (float) opacity->shadow);
    }
}
