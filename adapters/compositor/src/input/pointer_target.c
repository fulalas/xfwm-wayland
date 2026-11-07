#include <math.h>
#include <wlr/types/wlr_scene.h>

#include "osd/osd.h"
#include "pointer_target.h"
#include "render/scene_layers.h"
#include "seat.h"
#include "shell/view.h"

typedef struct scene_hit {
    struct wlr_scene_node *node;
    layout_point local;
} scene_hit;

static scene_hit node_at(compositor *server, layout_point point);
static scene_hit node_below_drag_icons(compositor *server, layout_point point);
static scene_hit scene_hit_in(struct wlr_scene_node *tree, layout_point point);
static void fill_surface(pointer_hit *hit, const scene_hit *scene);
static wm_pointer_target target_for_owner(compositor *server, const node_owner *owner,
                                          wm_point point);
static wm_pointer_target window_target(wm_target_kind kind, const node_owner *owner);

pointer_hit pointer_target_at(compositor *server, layout_point point)
{
    pointer_hit hit = {.target = {WM_TARGET_DESKTOP, NULL, -1}};
    scene_hit scene = node_at(server, point);
    if (scene.node == NULL) {
        return hit;
    }
    hit.node = scene.node;
    fill_surface(&hit, &scene);
    hit.owner = node_owner_find(scene.node);
    wm_point position = {(int) floor(point.x), (int) floor(point.y)};
    hit.target = target_for_owner(server, hit.owner, position);
    return hit;
}

static scene_hit node_at(compositor *server, layout_point point)
{
    if (seat_input_route(server->seat) == INPUT_ROUTE_LOCKED) {
        struct wlr_scene_node *lock = &scene_layers_tree(server->layers, SCENE_LAYER_LOCK)->node;
        return scene_hit_in(lock, point);
    }
    return node_below_drag_icons(server, point);
}

static scene_hit node_below_drag_icons(compositor *server, layout_point point)
{
    struct wlr_scene_node *drag = &scene_layers_tree(server->layers, SCENE_LAYER_DRAG)->node;
    struct wlr_scene_node *layer;
    wl_list_for_each_reverse(layer, &server->scene->tree.children, link) {
        if (layer == drag || !layer->enabled) {
            continue;
        }
        scene_hit hit = scene_hit_in(layer, point);
        if (hit.node != NULL) {
            return hit;
        }
    }
    return (scene_hit){0};
}

static scene_hit scene_hit_in(struct wlr_scene_node *tree, layout_point point)
{
    scene_hit hit = {0};
    hit.node = wlr_scene_node_at(tree, point.x, point.y, &hit.local.x, &hit.local.y);
    return hit;
}

static void fill_surface(pointer_hit *hit, const scene_hit *scene)
{
    if (scene->node->type != WLR_SCENE_NODE_BUFFER) {
        return;
    }
    struct wlr_scene_surface *scene_surface =
        wlr_scene_surface_try_from_buffer(wlr_scene_buffer_from_node(scene->node));
    if (scene_surface != NULL) {
        hit->surface = scene_surface->surface;
        hit->sx = scene->local.x;
        hit->sy = scene->local.y;
    }
}

static wm_pointer_target target_for_owner(compositor *server, const node_owner *owner,
                                          wm_point point)
{
    if (owner == NULL) {
        return (wm_pointer_target){WM_TARGET_DESKTOP, NULL, -1};
    }
    switch (owner->role) {
    case NODE_VIEW_CLIENT:
        return window_target(WM_TARGET_CLIENT, owner);
    case NODE_VIEW_FRAME:
        return window_target(WM_TARGET_FRAME, owner);
    case NODE_DESKTOP_SURFACE:
        return (wm_pointer_target){WM_TARGET_DESKTOP, NULL, -1};
    case NODE_OSD_MENU:
    case NODE_OSD_SUBMENU:
    case NODE_OSD_CYCLE:
        return osd_target_at(server->osd, owner->role, point);
    default:
        break;
    }
    return (wm_pointer_target){WM_TARGET_OTHER, NULL, -1};
}

static wm_pointer_target window_target(wm_target_kind kind, const node_owner *owner)
{
    return (wm_pointer_target){kind, view_window(owner->object), -1};
}
