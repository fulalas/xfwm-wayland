#include <stdlib.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>

#include "util/listener.h"
#include "util/node_owner.h"
#include "popup.h"
#include "render/scene_buffers.h"
#include "decor/shadow.h"
#include "render/surface_opacity.h"

typedef struct popup {
    compositor *server;
    struct wlr_xdg_popup *xdg_popup;
    struct wlr_scene_tree *wrap;
    struct wlr_scene_tree *content;
    shadow *shadow;
    node_owner owner;
    popup_parent as_parent;
    struct wl_listener commit;
    struct wl_listener reposition;
    struct wl_listener destroy;
    struct wl_listener wrap_destroy;
} popup;

static void build_scene(popup *popup, const popup_parent *parent);
static void connect_events(popup *popup);
static void handle_commit(struct wl_listener *listener, void *data);
static void handle_reposition(struct wl_listener *listener, void *data);
static void handle_destroy(struct wl_listener *listener, void *data);
static void handle_wrap_destroy(struct wl_listener *listener, void *data);
static void unconstrain(popup *popup);
static void update_shadow(popup *popup);

void popup_create(compositor *server, struct wlr_xdg_popup *xdg_popup, const popup_parent *parent)
{
    if (parent->tree == NULL) {
        wlr_log(WLR_ERROR, "cannot show a popup whose parent is gone");
        return;
    }
    popup *p = calloc(1, sizeof(*p));
    if (p == NULL) {
        return;
    }
    p->server = server;
    p->xdg_popup = xdg_popup;
    build_scene(p, parent);
    p->owner = (node_owner){NODE_POPUP, p};
    p->as_parent = (popup_parent){p->content, parent->root, parent->constraint};
    xdg_popup->base->data = &p->as_parent;
    connect_events(p);
}

static void build_scene(popup *p, const popup_parent *parent)
{
    p->wrap = wlr_scene_tree_create(parent->tree);
    p->shadow = p->wrap != NULL ? shadow_create(p->wrap) : NULL;
    p->content =
        p->wrap != NULL ? wlr_scene_xdg_surface_create(p->wrap, p->xdg_popup->base) : NULL;
    if (p->content == NULL || p->shadow == NULL) {
        wlr_log(WLR_ERROR, "cannot create the scene of a popup");
    }
}

static void connect_events(popup *p)
{
    struct wlr_xdg_popup *xdg_popup = p->xdg_popup;
    if (p->wrap != NULL) {
        node_owner_attach(&p->wrap->node, &p->owner);
        listener_connect(&p->wrap_destroy, &p->wrap->node.events.destroy, handle_wrap_destroy);
    }
    listener_connect(&p->commit, &xdg_popup->base->surface->events.commit, handle_commit);
    listener_connect(&p->reposition, &xdg_popup->events.reposition, handle_reposition);
    listener_connect(&p->destroy, &xdg_popup->events.destroy, handle_destroy);
}

void popup_style_buffers(compositor *server, struct wlr_scene_tree *tree)
{
    int percent = server->config.compositing.popup_opacity_percent;
    scene_buffers_set_opacity(tree, (float) scene_opacity_from_percent(percent));
}

static void handle_commit(struct wl_listener *listener, void *data)
{
    (void) data;
    popup *p = wl_container_of(listener, p, commit);
    if (p->xdg_popup->base->initial_commit) {
        unconstrain(p);
    }
    if (p->content != NULL) {
        popup_style_buffers(p->server, p->content);
        update_shadow(p);
    }
}

static void handle_reposition(struct wl_listener *listener, void *data)
{
    (void) data;
    popup *p = wl_container_of(listener, p, reposition);
    unconstrain(p);
}

static void handle_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    popup *p = wl_container_of(listener, p, destroy);
    listener_disconnect(&p->commit);
    listener_disconnect(&p->reposition);
    listener_disconnect(&p->destroy);
    if (p->wrap != NULL) {
        listener_disconnect(&p->wrap_destroy);
        if (p->shadow != NULL) {
            shadow_destroy(p->shadow);
        }
        wlr_scene_node_destroy(&p->wrap->node);
    } else if (p->shadow != NULL) {
        shadow_release(p->shadow);
    }
    free(p);
}

static void handle_wrap_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    popup *p = wl_container_of(listener, p, wrap_destroy);
    listener_disconnect(&p->wrap_destroy);
    p->wrap = NULL;
    p->content = NULL;
    p->as_parent.tree = NULL;
}

static void unconstrain(popup *p)
{
    if (p->as_parent.constraint == NULL) {
        return;
    }
    wm_rect area = p->as_parent.constraint(p->as_parent.root);
    struct wlr_box box = {area.x, area.y, area.width, area.height};
    wlr_xdg_popup_unconstrain_from_box(p->xdg_popup, &box);
}

static void update_shadow(popup *p)
{
    const wm_compositing_config *config = &p->server->config.compositing;
    if (p->shadow == NULL || !config->show_popup_shadow ||
        !surface_is_opaque(p->xdg_popup->base->surface)) {
        if (p->shadow != NULL) {
            shadow_hide(p->shadow);
        }
        return;
    }
    struct wlr_box geometry = p->xdg_popup->current.geometry;
    wm_rect frame = {geometry.x, geometry.y, geometry.width, geometry.height};
    shadow_show_around(p->shadow, p->server, &frame);
}
