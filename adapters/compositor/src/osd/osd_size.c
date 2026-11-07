#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wlr/types/wlr_scene.h>

#include "render/cairo_buffer.h"
#include "osd_geometry.h"
#include "osd_parts.h"
#include "output/output.h"
#include "render/scene_buffers.h"

enum {
    SIZE_TEXT_MAX = 32,
};

struct osd_size {
    compositor *server;
    struct wlr_scene_buffer *node;
    char text[SIZE_TEXT_MAX];
    wm_size size;
    bool painted;
};

static bool paint(osd_size *label);

osd_size *osd_size_create(compositor *server, struct wlr_scene_tree *parent)
{
    osd_size *label = calloc(1, sizeof(*label));
    if (label == NULL) {
        return NULL;
    }
    label->server = server;
    label->node = wlr_scene_buffer_create(parent, NULL);
    if (label->node == NULL) {
        free(label);
        return NULL;
    }
    label->node->point_accepts_input = scene_buffer_reject_input;
    wlr_scene_node_set_enabled(&label->node->node, false);
    return label;
}

void osd_size_destroy(osd_size *label)
{
    wlr_scene_node_destroy(&label->node->node);
    free(label);
}

void osd_size_show(osd_size *label, const wm_size_view *view)
{
    char text[SIZE_TEXT_MAX];
    snprintf(text, sizeof(text), "(%dx%d)", view->columns, view->rows);
    if (!label->painted || strcmp(text, label->text) != 0) {
        memcpy(label->text, text, sizeof(text));
        label->painted = paint(label);
    }
    wm_point origin = osd_center_origin(label->size, &view->frame);
    wlr_scene_node_set_position(&label->node->node, origin.x, origin.y);
    wlr_scene_node_set_enabled(&label->node->node, label->painted);
}

void osd_size_hide(osd_size *label)
{
    label->painted = false;
    wlr_scene_node_set_enabled(&label->node->node, false);
}

static bool paint(osd_size *label)
{
    const compositor_osd_painter *painter = osd_painter_of(label->server);
    label->size = painter->layout_label(painter->self, label->text);
    label_paint_request request = {
        .text = label->text,
        .size = label->size,
        .scale = outputs_max_scale(label->server->outputs),
    };
    cairo_surface_t *surface = painter->paint_label(painter->self, &request);
    return cairo_buffer_attach_sized(label->node, surface, label->size);
}
