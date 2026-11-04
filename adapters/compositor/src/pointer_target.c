#include <math.h>
#include <wlr/types/wlr_scene.h>

#include "osd.h"
#include "pointer_target.h"
#include "scene_layers.h"
#include "seat.h"
#include "view.h"

static struct wlr_scene_node *node_at(compositor *server, layout_point point, double *nx,
                                      double *ny);
static struct wlr_scene_node *node_below_drag_icons(compositor *server, layout_point point,
                                                   double *nx, double *ny);
static wm_pointer_target target_for_owner(compositor *server, const node_owner *owner,
                                          wm_point point);
static wm_pointer_target window_target(wm_target_kind kind, const node_owner *owner);

pointer_hit pointer_target_at(compositor *server, layout_point point)
{
    pointer_hit hit = {.target = {WM_TARGET_DESKTOP, NULL, -1}};
    double nx = 0;
    double ny = 0;
    struct wlr_scene_node *node = node_at(server, point, &nx, &ny);
    if (node == NULL) {
        return hit;
    }
    hit.node = node;
    if (node->type == WLR_SCENE_NODE_BUFFER) {
        struct wlr_scene_surface *scene_surface =
            wlr_scene_surface_try_from_buffer(wlr_scene_buffer_from_node(node));
        if (scene_surface != NULL) {
            hit.surface = scene_surface->surface;
            hit.sx = nx;
            hit.sy = ny;
        }
    }
    hit.owner = node_owner_find(node);
    wm_point position = {(int) floor(point.x), (int) floor(point.y)};
    hit.target = target_for_owner(server, hit.owner, position);
    return hit;
}

static struct wlr_scene_node *node_at(compositor *server, layout_point point, double *nx,
                                      double *ny)
{
    if (seat_input_route(server->seat) == INPUT_ROUTE_LOCKED) {
        struct wlr_scene_node *lock = &scene_layers_tree(server->layers, SCENE_LAYER_LOCK)->node;
        return wlr_scene_node_at(lock, point.x, point.y, nx, ny);
    }
    return node_below_drag_icons(server, point, nx, ny);
}

static struct wlr_scene_node *node_below_drag_icons(compositor *server, layout_point point,
                                                   double *nx, double *ny)
{
    struct wlr_scene_node *drag = &scene_layers_tree(server->layers, SCENE_LAYER_DRAG)->node;
    struct wlr_scene_node *layer;
    wl_list_for_each_reverse(layer, &server->scene->tree.children, link) {
        if (layer == drag || !layer->enabled) {
            continue;
        }
        struct wlr_scene_node *node = wlr_scene_node_at(layer, point.x, point.y, nx, ny);
        if (node != NULL) {
            return node;
        }
    }
    return NULL;
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
