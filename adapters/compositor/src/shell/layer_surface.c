#include <stdlib.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_scene.h>

#include "layer_shell_private.h"
#include "util/listener.h"
#include "render/scene_layers.h"
#include "input/seat.h"

static bool build_scene(layer_surface *surface);
static void free_scene(layer_surface *surface);
static void connect_listeners(layer_surface *surface);
static void handle_map(struct wl_listener *listener, void *data);
static void handle_unmap(struct wl_listener *listener, void *data);
static void handle_commit(struct wl_listener *listener, void *data);
static void handle_destroy(struct wl_listener *listener, void *data);
static void handle_new_popup(struct wl_listener *listener, void *data);
static void move_to_layer(layer_surface *surface, scene_layer layer);
static node_role role_for(const layer_surface *surface);
static void update_shadow(layer_surface *surface);
static bool wants_shadow(const layer_surface *surface);
static wm_rect popup_constraint(void *root);
static struct wlr_box to_box(wm_rect rect);

layer_surface *layer_surface_create(layer_shell *shell, struct wlr_layer_surface_v1 *wlr)
{
    layer_surface *ls = calloc(1, sizeof(*ls));
    if (ls == NULL) {
        return NULL;
    }
    ls->shell = shell;
    ls->wlr = wlr;
    ls->output = output_from_wlr(wlr->output);
    ls->layer = layer_mapping_from_shell(wlr->pending.layer);
    if (!build_scene(ls)) {
        free_scene(ls);
        free(ls);
        return NULL;
    }
    ls->owner = (node_owner){role_for(ls), ls};
    node_owner_attach(&ls->tree->node, &ls->owner);
    ls->popup_parent = (popup_parent){ls->popups, ls, popup_constraint};
    wlr->data = ls;
    connect_listeners(ls);
    wl_list_insert(&shell->surfaces, &ls->link);
    return ls;
}

wm_rect layer_surface_configure(layer_surface *ls, const layer_area *area)
{
    struct wlr_box full = to_box(area->full);
    struct wlr_box usable = to_box(area->usable);
    wlr_scene_layer_surface_v1_configure(ls->scene, &full, &usable);
    struct wlr_scene_node *node = &ls->scene->tree->node;
    wlr_scene_node_set_position(&ls->popups->node, node->x, node->y);
    update_shadow(ls);
    return (wm_rect){usable.x, usable.y, usable.width, usable.height};
}

static bool build_scene(layer_surface *ls)
{
    const scene_layers *layers = ls->shell->server->layers;
    ls->tree = wlr_scene_tree_create(scene_layers_tree(layers, ls->layer));
    if (ls->tree == NULL) {
        return false;
    }
    ls->shadow = shadow_create(ls->tree);
    ls->scene = wlr_scene_layer_surface_v1_create(ls->tree, ls->wlr);
    ls->popups = wlr_scene_tree_create(scene_layers_tree(layers, SCENE_LAYER_POPUPS));
    return ls->shadow != NULL && ls->scene != NULL && ls->popups != NULL;
}

static void free_scene(layer_surface *ls)
{
    if (ls->shadow != NULL) {
        shadow_destroy(ls->shadow);
    }
    if (ls->popups != NULL) {
        wlr_scene_node_destroy(&ls->popups->node);
    }
    if (ls->tree != NULL) {
        wlr_scene_node_destroy(&ls->tree->node);
    }
}

static void connect_listeners(layer_surface *ls)
{
    struct wlr_surface *surface = ls->wlr->surface;
    listener_connect(&ls->map, &surface->events.map, handle_map);
    listener_connect(&ls->unmap, &surface->events.unmap, handle_unmap);
    listener_connect(&ls->commit, &surface->events.commit, handle_commit);
    listener_connect(&ls->destroy, &ls->wlr->events.destroy, handle_destroy);
    listener_connect(&ls->new_popup, &ls->wlr->events.new_popup, handle_new_popup);
}

static void handle_map(struct wl_listener *listener, void *data)
{
    (void) data;
    layer_surface *ls = wl_container_of(listener, ls, map);
    compositor *server = ls->shell->server;
    outputs_schedule_arrange(server->outputs);
    seat_focus_refresh(server->seat);
}

static void handle_unmap(struct wl_listener *listener, void *data)
{
    (void) data;
    layer_surface *ls = wl_container_of(listener, ls, unmap);
    compositor *server = ls->shell->server;
    shadow_hide(ls->shadow);
    seat_surface_gone(server->seat, ls->wlr->surface);
    outputs_schedule_arrange(server->outputs);
}

static void handle_commit(struct wl_listener *listener, void *data)
{
    (void) data;
    layer_surface *ls = wl_container_of(listener, ls, commit);
    struct wlr_layer_surface_v1 *wlr = ls->wlr;
    if (!wlr->initialized) {
        return;
    }
    if ((wlr->current.committed & WLR_LAYER_SURFACE_V1_STATE_LAYER) != 0) {
        move_to_layer(ls, layer_mapping_from_shell(wlr->current.layer));
    }
    if (wlr->initial_commit || wlr->current.committed != 0) {
        outputs_schedule_arrange(ls->shell->server->outputs);
    }
}

static void handle_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    layer_surface *ls = wl_container_of(listener, ls, destroy);
    listener_disconnect(&ls->map);
    listener_disconnect(&ls->unmap);
    listener_disconnect(&ls->commit);
    listener_disconnect(&ls->destroy);
    listener_disconnect(&ls->new_popup);
    wl_list_remove(&ls->link);
    free_scene(ls);
    compositor *server = ls->shell->server;
    free(ls);
    outputs_schedule_arrange(server->outputs);
}

static void handle_new_popup(struct wl_listener *listener, void *data)
{
    layer_surface *ls = wl_container_of(listener, ls, new_popup);
    popup_create(ls->shell->server, data, &ls->popup_parent);
}

static void move_to_layer(layer_surface *ls, scene_layer layer)
{
    if (layer == ls->layer) {
        return;
    }
    ls->layer = layer;
    ls->owner.role = role_for(ls);
    wlr_scene_node_reparent(&ls->tree->node, scene_layers_tree(ls->shell->server->layers, layer));
}

static node_role role_for(const layer_surface *ls)
{
    return layer_mapping_is_desktop(ls->layer, ls->wlr->namespace) ? NODE_DESKTOP_SURFACE
                                                                   : NODE_LAYER_SURFACE;
}

static void update_shadow(layer_surface *ls)
{
    if (!wants_shadow(ls)) {
        shadow_hide(ls->shadow);
        return;
    }
    struct wlr_scene_node *node = &ls->scene->tree->node;
    wm_rect frame = {node->x, node->y, (int) ls->wlr->current.actual_width,
                     (int) ls->wlr->current.actual_height};
    shadow_show_around(ls->shadow, ls->shell->server, &frame);
}

static bool wants_shadow(const layer_surface *ls)
{
    bool dock_layer = ls->layer == SCENE_LAYER_TOP || ls->layer == SCENE_LAYER_BOTTOM;
    return dock_layer && ls->owner.role != NODE_DESKTOP_SURFACE && ls->wlr->surface->mapped &&
           ls->shell->server->config.compositing.show_dock_shadow;
}

static wm_rect popup_constraint(void *root)
{
    layer_surface *ls = root;
    if (ls->output == NULL) {
        return (wm_rect){0, 0, 0, 0};
    }
    wm_rect full = output_full(ls->output);
    struct wlr_scene_node *node = &ls->scene->tree->node;
    return (wm_rect){full.x - node->x, full.y - node->y, full.width, full.height};
}

static struct wlr_box to_box(wm_rect rect)
{
    return (struct wlr_box){rect.x, rect.y, rect.width, rect.height};
}
