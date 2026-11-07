#include <stdlib.h>
#include <string.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/xwayland/xwayland.h>

#include "util/node_owner.h"
#include "output/output.h"
#include "shell/popup.h"
#include "render/scene_layers.h"
#include "input/seat.h"
#include "decor/shadow.h"
#include "render/surface_opacity.h"
#include "unmanaged.h"

static const enum wlr_xwayland_net_wm_window_type popup_types[] = {
    WLR_XWAYLAND_NET_WM_WINDOW_TYPE_MENU,          WLR_XWAYLAND_NET_WM_WINDOW_TYPE_DROPDOWN_MENU,
    WLR_XWAYLAND_NET_WM_WINDOW_TYPE_POPUP_MENU,    WLR_XWAYLAND_NET_WM_WINDOW_TYPE_TOOLTIP,
    WLR_XWAYLAND_NET_WM_WINDOW_TYPE_COMBO,
};

typedef struct shadow_inputs {
    wm_compositing_config config;
    wm_rect geometry;
    bool opaque;
} shadow_inputs;

struct unmanaged {
    compositor *server;
    struct wlr_xwayland_surface *xsurface;
    struct wlr_scene_tree *tree;
    struct wlr_scene_tree *surface_tree;
    shadow *shadow;
    node_owner owner;
    bool popup_style;
    bool focused;
    bool styled;
    shadow_inputs styled_for;
};

static bool build_scene(unmanaged *unmanaged);
static bool is_popup(const struct wlr_xwayland_surface *xsurface);
static void apply_style(unmanaged *unmanaged);
static void show_style(unmanaged *unmanaged, const shadow_inputs *inputs);

unmanaged *unmanaged_create(compositor *server, struct wlr_xwayland_surface *xsurface)
{
    unmanaged *u = calloc(1, sizeof(*u));
    if (u == NULL) {
        return NULL;
    }
    u->server = server;
    u->xsurface = xsurface;
    if (!build_scene(u)) {
        unmanaged_destroy(u);
        return NULL;
    }
    u->owner = (node_owner){NODE_UNMANAGED, u};
    node_owner_attach(&u->tree->node, &u->owner);
    u->popup_style = is_popup(xsurface);
    unmanaged_move(u);
    if (wlr_xwayland_surface_override_redirect_wants_focus(xsurface)) {
        u->focused = true;
        seat_focus_unmanaged(server->seat, xsurface->surface);
    }
    return u;
}

void unmanaged_destroy(unmanaged *u)
{
    if (u->focused) {
        seat_surface_gone(u->server->seat, u->xsurface->surface);
    }
    if (u->shadow != NULL) {
        shadow_destroy(u->shadow);
    }
    if (u->tree != NULL) {
        wlr_scene_node_destroy(&u->tree->node);
    }
    free(u);
}

void unmanaged_move(unmanaged *u)
{
    wlr_scene_node_set_position(&u->tree->node, u->xsurface->x, u->xsurface->y);
    apply_style(u);
}

void unmanaged_committed(unmanaged *u)
{
    apply_style(u);
}

static bool build_scene(unmanaged *u)
{
    u->tree = wlr_scene_tree_create(scene_layers_tree(u->server->layers, SCENE_LAYER_UNMANAGED));
    if (u->tree == NULL) {
        return false;
    }
    u->shadow = shadow_create(u->tree);
    u->surface_tree = wlr_scene_subsurface_tree_create(u->tree, u->xsurface->surface);
    return u->shadow != NULL && u->surface_tree != NULL;
}

static bool is_popup(const struct wlr_xwayland_surface *xsurface)
{
    for (size_t i = 0; i < sizeof(popup_types) / sizeof(popup_types[0]); i++) {
        if (wlr_xwayland_surface_has_window_type(xsurface, popup_types[i])) {
            return true;
        }
    }
    return false;
}

static bool covers_output(const unmanaged *u)
{
    const struct wlr_xwayland_surface *xs = u->xsurface;
    wm_rect window = {xs->x, xs->y, xs->width, xs->height};
    output *o = outputs_near_point(u->server->outputs, wm_rect_center(&window));
    if (o == NULL) {
        return false;
    }
    wm_rect full = output_full(o);
    wm_rect covered = wm_rect_intersection(&window, &full);
    return wm_rect_equals(&covered, &full);
}

static void apply_style(unmanaged *u)
{
    const wm_compositing_config *config = &u->server->config.compositing;
    if (u->popup_style) {
        popup_style_buffers(u->server, u->surface_tree);
    }
    const struct wlr_xwayland_surface *xs = u->xsurface;
    shadow_inputs inputs;
    memset(&inputs, 0, sizeof(inputs));
    inputs.config = *config;
    inputs.geometry = (wm_rect){xs->x, xs->y, xs->width, xs->height};
    inputs.opaque = surface_is_opaque(xs->surface);
    if (u->styled && memcmp(&inputs, &u->styled_for, sizeof(inputs)) == 0) {
        return;
    }
    show_style(u, &inputs);
}

static void show_style(unmanaged *u, const shadow_inputs *inputs)
{
    u->styled = true;
    u->styled_for = *inputs;
    if (!inputs->config.show_popup_shadow || !inputs->opaque || covers_output(u)) {
        shadow_hide(u->shadow);
        return;
    }
    wm_rect frame = {0, 0, u->xsurface->width, u->xsurface->height};
    shadow_show_around(u->shadow, u->server, &frame);
}
