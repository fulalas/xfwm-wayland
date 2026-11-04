#include <stdlib.h>
#include <wlr/types/wlr_buffer.h>
#include <wlr/types/wlr_scene.h>

#include "cairo_buffer.h"
#include "osd_parts.h"
#include "scene_buffers.h"

enum {
    OUTLINE_WIDTH = 2,
    OUTLINE_SIDES = 4,
};

static const uint32_t OUTLINE_COLOR = 0xffffffff;

struct osd_outline {
    struct wlr_scene_tree *tree;
    struct wlr_scene_buffer *sides[OUTLINE_SIDES];
};

static void place_side(struct wlr_scene_buffer *side, wm_rect rect);

osd_outline *osd_outline_create(struct wlr_scene_tree *parent)
{
    osd_outline *outline = calloc(1, sizeof(*outline));
    if (outline == NULL) {
        return NULL;
    }
    outline->tree = wlr_scene_tree_create(parent);
    struct wlr_buffer *color = cairo_buffer_create_solid(OUTLINE_COLOR);
    for (int i = 0; i < OUTLINE_SIDES && outline->tree != NULL; i++) {
        outline->sides[i] = wlr_scene_buffer_create(outline->tree, color);
        if (outline->sides[i] != NULL) {
            outline->sides[i]->point_accepts_input = scene_buffer_reject_input;
        }
    }
    if (color != NULL) {
        wlr_buffer_drop(color);
    }
    if (outline->tree == NULL) {
        free(outline);
        return NULL;
    }
    wlr_scene_node_set_enabled(&outline->tree->node, false);
    return outline;
}

void osd_outline_destroy(osd_outline *outline)
{
    wlr_scene_node_destroy(&outline->tree->node);
    free(outline);
}

void osd_outline_show(osd_outline *outline, const wm_rect *r)
{
    int w = OUTLINE_WIDTH;
    place_side(outline->sides[0], (wm_rect){r->x, r->y, r->width, w});
    place_side(outline->sides[1], (wm_rect){r->x, r->y + r->height - w, r->width, w});
    place_side(outline->sides[2], (wm_rect){r->x, r->y, w, r->height});
    place_side(outline->sides[3], (wm_rect){r->x + r->width - w, r->y, w, r->height});
    wlr_scene_node_set_enabled(&outline->tree->node, true);
}

void osd_outline_hide(osd_outline *outline)
{
    wlr_scene_node_set_enabled(&outline->tree->node, false);
}

static void place_side(struct wlr_scene_buffer *side, wm_rect rect)
{
    if (side == NULL) {
        return;
    }
    bool visible = rect.width > 0 && rect.height > 0;
    wlr_scene_node_set_enabled(&side->node, visible);
    if (visible) {
        wlr_scene_buffer_set_dest_size(side, rect.width, rect.height);
        wlr_scene_node_set_position(&side->node, rect.x, rect.y);
    }
}
