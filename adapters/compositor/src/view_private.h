#ifndef COMPOSITOR_VIEW_PRIVATE_H
#define COMPOSITOR_VIEW_PRIVATE_H

#include "decorations.h"
#include "layer_mapping.h"
#include "node_owner.h"
#include "shadow.h"
#include "view.h"

enum {
    VIEW_TITLE_MAX = 256,
    VIEW_APP_ID_MAX = 256,
};

typedef struct view_target {
    wm_window_view state;
    wm_decor_layout decor;
    char title[VIEW_TITLE_MAX];
    char app_id[VIEW_APP_ID_MAX];
} view_target;

typedef struct view_handshake {
    bool awaiting;
    wm_size requested;
    wm_size committed;
    struct wl_event_source *timeout;
} view_handshake;

typedef struct view_paint_mark {
    bool painted;
    unsigned serial;
    double scale;
    wm_point origin;
} view_paint_mark;

struct view {
    compositor *server;
    const view_impl *impl;
    void *self;
    wm_window *window;
    node_owner client_owner;
    node_owner frame_owner;
    struct wlr_scene_tree *tree;
    struct wlr_scene_tree *content;
    struct wlr_scene_tree *popups;
    decorations *decor;
    shadow *shadow;
    view_target target;
    wm_rect shown_frame;
    bool presented;
    view_handshake handshake;
    view_paint_mark paint_mark;
    scene_layer layer;
    struct wl_list link;
};

void view_apply_geometry(view *view);
void view_apply_content_opacity(view *view);

#endif
