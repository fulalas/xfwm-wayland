#ifndef COMPOSITOR_NODE_OWNER_H
#define COMPOSITOR_NODE_OWNER_H

struct wlr_scene_node;

typedef enum node_role {
    NODE_VIEW_CLIENT,
    NODE_VIEW_FRAME,
    NODE_POPUP,
    NODE_LAYER_SURFACE,
    NODE_DESKTOP_SURFACE,
    NODE_UNMANAGED,
    NODE_OSD_MENU,
    NODE_OSD_SUBMENU,
    NODE_OSD_CYCLE,
    NODE_LOCK_SURFACE,
} node_role;

typedef struct node_owner {
    node_role role;
    void *object;
} node_owner;

void node_owner_attach(struct wlr_scene_node *node, node_owner *owner);
const node_owner *node_owner_find(struct wlr_scene_node *node);

#endif
