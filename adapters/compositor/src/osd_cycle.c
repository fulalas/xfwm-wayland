#include <stdio.h>
#include <stdlib.h>
#include <wlr/types/wlr_buffer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_scene.h>

#include "cairo_buffer.h"
#include "osd_geometry.h"
#include "osd_parts.h"
#include "osd_preview.h"
#include "output.h"

enum {
    CYCLE_TITLE_MAX = 128,
};

struct osd_cycle {
    compositor *server;
    node_owner owner;
    struct wlr_scene_tree *tree;
    struct wlr_scene_buffer *panel;
    osd_preview *previews[WM_CYCLE_ITEM_MAX];
    wm_cycle_view view;
    char titles[WM_CYCLE_ITEM_MAX][CYCLE_TITLE_MAX];
    cycle_layout layout;
    wm_point origin;
    bool shown;
};

static void store_view(osd_cycle *cycle, const wm_cycle_view *view);
static void measure(osd_cycle *cycle);
static const compositor_osd_painter *osd_painter(const osd_cycle *cycle);
static void paint_panel(osd_cycle *cycle);
static void collect_icons(const osd_cycle *cycle, cairo_surface_t **icons);
static void update_previews(osd_cycle *cycle);
static void update_preview(osd_cycle *cycle, size_t index);
static osd_preview *preview_slot(osd_cycle *cycle, size_t index);
static void place_preview(osd_cycle *cycle, size_t index, struct wlr_scene_buffer *node);
static void clear_previews(osd_cycle *cycle);
static void destroy_previews(osd_cycle *cycle);

osd_cycle *osd_cycle_create(compositor *server, struct wlr_scene_tree *parent)
{
    osd_cycle *cycle = calloc(1, sizeof(*cycle));
    if (cycle == NULL) {
        return NULL;
    }
    cycle->server = server;
    cycle->owner = (node_owner){NODE_OSD_CYCLE, cycle};
    cycle->tree = wlr_scene_tree_create(parent);
    cycle->panel = cycle->tree != NULL ? wlr_scene_buffer_create(cycle->tree, NULL) : NULL;
    if (cycle->panel == NULL) {
        osd_cycle_destroy(cycle);
        return NULL;
    }
    node_owner_attach(&cycle->tree->node, &cycle->owner);
    wlr_scene_node_set_enabled(&cycle->tree->node, false);
    return cycle;
}

void osd_cycle_destroy(osd_cycle *cycle)
{
    destroy_previews(cycle);
    if (cycle->tree != NULL) {
        wlr_scene_node_destroy(&cycle->tree->node);
    }
    free(cycle);
}

void osd_cycle_show(osd_cycle *cycle, const wm_cycle_view *view)
{
    store_view(cycle, view);
    measure(cycle);
    paint_panel(cycle);
    clear_previews(cycle);
    if (cycle->view.preview) {
        update_previews(cycle);
    }
    cycle->shown = true;
    wlr_scene_node_set_enabled(&cycle->tree->node, true);
}

void osd_cycle_hide(osd_cycle *cycle)
{
    cycle->shown = false;
    clear_previews(cycle);
    wlr_scene_node_set_enabled(&cycle->tree->node, false);
}

bool osd_cycle_is_shown(const osd_cycle *cycle)
{
    return cycle->shown;
}

int osd_cycle_item_at(const osd_cycle *cycle, wm_point point)
{
    if (!cycle->shown) {
        return -1;
    }
    wm_point local = {point.x - cycle->origin.x, point.y - cycle->origin.y};
    return osd_item_at(cycle->layout.items, cycle->layout.count, local);
}

void osd_cycle_view_committed(osd_cycle *cycle, view *v)
{
    if (!cycle->shown || !cycle->view.preview) {
        return;
    }
    for (size_t i = 0; i < cycle->view.count; i++) {
        if (cycle->view.items[i].window == v) {
            update_preview(cycle, i);
        }
    }
}

void osd_cycle_view_destroyed(osd_cycle *cycle, view *v)
{
    for (size_t i = 0; i < cycle->view.count; i++) {
        if (cycle->view.items[i].window != v) {
            continue;
        }
        cycle->view.items[i].window = NULL;
        if (cycle->previews[i] != NULL) {
            osd_preview_clear(cycle->previews[i]);
        }
    }
}

void osd_cycle_repaint(osd_cycle *cycle)
{
    if (cycle->shown) {
        measure(cycle);
        paint_panel(cycle);
    }
}

static void store_view(osd_cycle *cycle, const wm_cycle_view *view)
{
    cycle->view = *view;
    size_t count = view->count < WM_CYCLE_ITEM_MAX ? view->count : WM_CYCLE_ITEM_MAX;
    cycle->view.count = count;
    for (size_t i = 0; i < count; i++) {
        const char *title = view->items[i].title;
        snprintf(cycle->titles[i], CYCLE_TITLE_MAX, "%s", title != NULL ? title : "");
        cycle->view.items[i].title = cycle->titles[i];
    }
}

static void measure(osd_cycle *cycle)
{
    const compositor_osd_painter *painter = osd_painter(cycle);
    cycle->layout = painter->layout_cycle(painter->self, &cycle->view);
    cycle->origin = osd_center_origin(cycle->layout.size, &cycle->view.monitor);
}

static const compositor_osd_painter *osd_painter(const osd_cycle *cycle)
{
    return &cycle->server->painter->osd;
}

static void paint_panel(osd_cycle *cycle)
{
    const compositor_osd_painter *painter = osd_painter(cycle);
    cairo_surface_t *icons[WM_CYCLE_ITEM_MAX] = {0};
    collect_icons(cycle, icons);
    cycle_paint_request request = {
        .view = &cycle->view,
        .layout = &cycle->layout,
        .icons = icons,
        .scale = outputs_max_scale(cycle->server->outputs),
    };
    cairo_surface_t *surface = painter->paint_cycle(painter->self, &request);
    cairo_buffer_attach_sized(cycle->panel, surface, cycle->layout.size);
    wlr_scene_node_set_position(&cycle->panel->node, cycle->origin.x, cycle->origin.y);
}

static void collect_icons(const osd_cycle *cycle, cairo_surface_t **icons)
{
    for (size_t i = 0; i < cycle->view.count; i++) {
        view *v = cycle->view.items[i].window;
        icons[i] = v != NULL ? view_icon(v) : NULL;
    }
}

static void update_previews(osd_cycle *cycle)
{
    for (size_t i = 0; i < cycle->view.count; i++) {
        update_preview(cycle, i);
    }
}

static void update_preview(osd_cycle *cycle, size_t index)
{
    view *v = cycle->view.items[index].window;
    osd_preview *preview = preview_slot(cycle, index);
    if (preview == NULL) {
        return;
    }
    struct wlr_scene_buffer *node = osd_preview_node(preview);
    bool shown = v != NULL && osd_preview_show(preview, view_surface(v));
    wlr_scene_node_set_enabled(&node->node, shown);
    if (shown) {
        place_preview(cycle, index, node);
    }
}

static osd_preview *preview_slot(osd_cycle *cycle, size_t index)
{
    if (cycle->previews[index] == NULL) {
        cycle->previews[index] = osd_preview_create(cycle->server, cycle->tree);
    }
    return cycle->previews[index];
}

static void place_preview(osd_cycle *cycle, size_t index, struct wlr_scene_buffer *node)
{
    wm_size buffer_size = {node->buffer->width, node->buffer->height};
    wm_rect fitted = osd_fit_preview(buffer_size, &cycle->layout.previews[index]);
    wlr_scene_buffer_set_dest_size(node, fitted.width, fitted.height);
    wlr_scene_node_set_position(&node->node, cycle->origin.x + fitted.x,
                                cycle->origin.y + fitted.y);
}

static void clear_previews(osd_cycle *cycle)
{
    for (size_t i = 0; i < WM_CYCLE_ITEM_MAX; i++) {
        if (cycle->previews[i] != NULL) {
            osd_preview_clear(cycle->previews[i]);
            wlr_scene_node_set_enabled(&osd_preview_node(cycle->previews[i])->node, false);
        }
    }
}

static void destroy_previews(osd_cycle *cycle)
{
    for (size_t i = 0; i < WM_CYCLE_ITEM_MAX; i++) {
        if (cycle->previews[i] != NULL) {
            osd_preview_destroy(cycle->previews[i]);
            cycle->previews[i] = NULL;
        }
    }
}
