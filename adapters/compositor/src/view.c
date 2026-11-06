#include <stdlib.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/util/log.h>

#include "surface_opacity.h"
#include "input_mapping.h"
#include "osd.h"
#include "scene_layers.h"
#include "seat.h"
#include "view_private.h"

enum {
    CONFIGURE_TIMEOUT_MS = 100,
};

static void begin_handshake(view *view);
static void end_handshake(view *view);
static int handle_handshake_timeout(void *data);
static void report_committed_size(view *view, wm_size size);
static wm_size rect_size(const wm_rect *rect);
static bool view_build_scene(view *view);
static void view_free_scene(view *view);

view *view_create(compositor *server, const view_impl *impl, void *self)
{
    view *v = calloc(1, sizeof(*v));
    if (v == NULL) {
        return NULL;
    }
    v->server = server;
    v->impl = impl;
    v->self = self;
    v->layer = SCENE_LAYER_NORMAL;
    v->target.state.opacity = (wm_opacity_view){1.0, 1.0, 1.0};
    v->target.state.title = v->target.title;
    v->target.state.app_id = v->target.app_id;
    v->client_owner = (node_owner){NODE_VIEW_CLIENT, v};
    v->frame_owner = (node_owner){NODE_VIEW_FRAME, v};
    if (!view_build_scene(v)) {
        view_free_scene(v);
        free(v);
        return NULL;
    }
    v->handshake.timeout = wl_event_loop_add_timer(server->loop, handle_handshake_timeout, v);
    wl_list_insert(&server->views, &v->link);
    return v;
}

void view_destroy(view *v)
{
    compositor *server = v->server;
    if (v->window != NULL) {
        wm_window *window = v->window;
        v->window = NULL;
        wm_window_unmap(server->desktop, window);
        wl_signal_emit(&server->view_events.unmapped, v);
    }
    seat_view_destroyed(server->seat, v);
    osd_view_destroyed(server->osd, v);
    if (v->handshake.timeout != NULL) {
        wl_event_source_remove(v->handshake.timeout);
    }
    view_free_scene(v);
    wl_list_remove(&v->link);
    free(v);
}

void view_map(view *v, const wm_window_desc *desc)
{
    compositor *server = v->server;
    wm_window_desc copy = *desc;
    copy.handle = v;
    copy.has_icon = view_icon(v) != NULL;
    v->handshake.committed = rect_size(&desc->geometry);
    v->handshake.requested = v->handshake.committed;
    v->window = wm_window_map(server->desktop, &copy);
    if (v->window == NULL) {
        wlr_log(WLR_ERROR, "cannot manage a new window");
        return;
    }
    wl_signal_emit(&server->view_events.mapped, v);
}

void view_configure(view *v, const wm_rect *client)
{
    view_handshake *h = &v->handshake;
    h->requested = rect_size(client);
    if (!wm_size_equals(h->requested, h->committed)) {
        begin_handshake(v);
    }
    v->impl->configure(v->self, client);
}

void view_committed(view *v, wm_size size)
{
    view_handshake *h = &v->handshake;
    bool changed = !wm_size_equals(size, h->committed);
    h->committed = size;
    if (changed && v->window != NULL) {
        wm_window_content_drawn(v->server->desktop, v->window, size);
    }
    if (v->window != NULL) {
        wm_window_set_opaque(v->server->desktop, v->window,
                             surface_is_opaque(v->impl->surface(v->self)));
    }
    if (changed && !h->awaiting) {
        report_committed_size(v, size);
    }
    if (v->target.state.opacity.content != 1.0) {
        view_apply_content_opacity(v);
    }
    osd_view_committed(v->server->osd, v);
}

void view_configure_settled(view *v)
{
    view_handshake *h = &v->handshake;
    if (!h->awaiting) {
        return;
    }
    end_handshake(v);
    if (!wm_size_equals(h->committed, h->requested)) {
        report_committed_size(v, h->committed);
    }
    if (v->presented) {
        view_apply_geometry(v);
    }
}

void view_request_state(view *v, const wm_state_change *change)
{
    if (v->window != NULL) {
        wm_request_state(v->server->desktop, v->window, change);
    }
}

void view_request_move(view *v)
{
    if (v->window != NULL) {
        seat_release_pointer_grab(v->server->seat);
        wm_request_move(v->server->desktop, v->window);
    }
}

void view_request_resize(view *v, uint32_t wlr_edges)
{
    if (v->window != NULL) {
        seat_release_pointer_grab(v->server->seat);
        wm_request_resize(v->server->desktop, v->window, input_mapping_edges_from_wlr(wlr_edges));
    }
}

wm_state_change view_state_change(uint32_t states, bool enabled)
{
    return (wm_state_change){enabled ? states : 0, enabled ? 0 : states};
}

void view_icon_changed(view *v)
{
    if (v->window != NULL) {
        wm_window_icon_changed(v->server->desktop, v->window);
    }
    osd_view_icon_changed(v->server->osd, v);
}

void view_report_not_responding(view *v)
{
    if (v->window != NULL) {
        wm_window_not_responding(v->server->desktop, v->window);
    }
}

static void begin_handshake(view *v)
{
    view_handshake *h = &v->handshake;
    if (h->awaiting) {
        return;
    }
    h->awaiting = true;
    if (h->timeout != NULL) {
        wl_event_source_timer_update(h->timeout, CONFIGURE_TIMEOUT_MS);
    }
}

static void end_handshake(view *v)
{
    view_handshake *h = &v->handshake;
    h->awaiting = false;
    if (h->timeout != NULL) {
        wl_event_source_timer_update(h->timeout, 0);
    }
}

static int handle_handshake_timeout(void *data)
{
    view *v = data;
    end_handshake(v);
    if (v->presented) {
        view_apply_geometry(v);
    }
    return 0;
}

static void report_committed_size(view *v, wm_size size)
{
    if (v->window != NULL) {
        wm_window_size_committed(v->server->desktop, v->window, size);
    }
}

static wm_size rect_size(const wm_rect *rect)
{
    return (wm_size){rect->width, rect->height};
}

static bool view_build_scene(view *v)
{
    compositor *server = v->server;
    v->tree = wlr_scene_tree_create(scene_layers_tree(server->layers, SCENE_LAYER_NORMAL));
    if (v->tree == NULL) {
        return false;
    }
    wlr_scene_node_set_enabled(&v->tree->node, false);
    v->shadow = shadow_create(v->tree);
    v->decor = decorations_create(v->tree, server->painter);
    v->content = wlr_scene_tree_create(v->tree);
    v->popups = wlr_scene_tree_create(v->tree);
    if (v->shadow == NULL || v->decor == NULL || v->content == NULL || v->popups == NULL) {
        return false;
    }
    node_owner_attach(&decorations_tree(v->decor)->node, &v->frame_owner);
    node_owner_attach(&v->content->node, &v->client_owner);
    return true;
}

static void view_free_scene(view *v)
{
    if (v->shadow != NULL) {
        shadow_destroy(v->shadow);
    }
    if (v->decor != NULL) {
        decorations_destroy(v->decor);
    }
    if (v->tree != NULL) {
        wlr_scene_node_destroy(&v->tree->node);
    }
}
