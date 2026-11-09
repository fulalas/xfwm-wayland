#ifndef COMPOSITOR_POPUP_H
#define COMPOSITOR_POPUP_H

#include "server/server.h"
#include "wm/geometry.h"

struct wlr_scene_tree;
struct wlr_surface;
struct wlr_xdg_popup;

typedef struct popup_parent {
    struct wlr_scene_tree *tree;
    void *root;
    wm_rect (*constraint)(void *root);
    struct wlr_scene_tree *grab_tree;
} popup_parent;

void popup_create(compositor *server, struct wlr_xdg_popup *xdg_popup,
                  const popup_parent *parent);
struct wlr_surface *popup_grabbed(struct wlr_xdg_popup *xdg_popup);
void popup_style_buffers(compositor *server, struct wlr_scene_tree *tree);

#endif
