#ifndef COMPOSITOR_POINTER_TARGET_H
#define COMPOSITOR_POINTER_TARGET_H

#include "node_owner.h"
#include "server.h"
#include "wm/desktop.h"

struct wlr_scene_node;
struct wlr_surface;

typedef struct pointer_hit {
    wm_pointer_target target;
    const node_owner *owner;
    struct wlr_scene_node *node;
    struct wlr_surface *surface;
    double sx;
    double sy;
} pointer_hit;

typedef struct layout_point {
    double x;
    double y;
} layout_point;

pointer_hit pointer_target_at(compositor *server, layout_point point);

#endif
