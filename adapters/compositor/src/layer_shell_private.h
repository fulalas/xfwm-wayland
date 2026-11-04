#ifndef COMPOSITOR_LAYER_SHELL_PRIVATE_H
#define COMPOSITOR_LAYER_SHELL_PRIVATE_H

#include <wayland-server-core.h>

#include "layer_mapping.h"
#include "layer_shell.h"
#include "node_owner.h"
#include "popup.h"
#include "shadow.h"

struct wlr_layer_surface_v1;
struct wlr_scene_layer_surface_v1;

struct layer_shell {
    compositor *server;
    struct wlr_layer_shell_v1 *wlr_shell;
    struct wl_list surfaces;
    struct wl_listener new_surface;
};

struct layer_surface {
    layer_shell *shell;
    struct wlr_layer_surface_v1 *wlr;
    output *output;
    struct wlr_scene_tree *tree;
    struct wlr_scene_layer_surface_v1 *scene;
    struct wlr_scene_tree *popups;
    shadow *shadow;
    node_owner owner;
    popup_parent popup_parent;
    scene_layer layer;
    struct wl_list link;
    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener commit;
    struct wl_listener destroy;
    struct wl_listener new_popup;
};

layer_surface *layer_surface_create(layer_shell *shell, struct wlr_layer_surface_v1 *wlr);
typedef struct layer_area {
    wm_rect full;
    wm_rect usable;
} layer_area;

wm_rect layer_surface_configure(layer_surface *surface, const layer_area *area);

#endif
