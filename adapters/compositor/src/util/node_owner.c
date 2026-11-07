#include <wlr/types/wlr_scene.h>

#include "node_owner.h"

void node_owner_attach(struct wlr_scene_node *node, node_owner *owner)
{
    node->data = owner;
}

const node_owner *node_owner_find(struct wlr_scene_node *node)
{
    for (struct wlr_scene_node *current = node; current != NULL;
         current = current->parent != NULL ? &current->parent->node : NULL) {
        if (current->data != NULL) {
            return current->data;
        }
    }
    return NULL;
}
