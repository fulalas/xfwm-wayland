#include <stdlib.h>
#include <wlr/types/wlr_buffer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_scene.h>

#include "render/cairo_buffer.h"
#include "osd_geometry.h"
#include "osd_parts.h"
#include "osd_preview.h"
#include "output/output.h"
#include "util/utf8_copy.h"

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
    char app_names[WM_CYCLE_ITEM_MAX][CYCLE_TITLE_MAX];
    cycle_layout layout;
    wm_point origin;
    bool shown;
    struct wl_event_source *repaint_idle;
};

static void store_view(osd_cycle *cycle, const wm_cycle_view *view);
static void measure(osd_cycle *cycle);
static void paint_panel(osd_cycle *cycle);
static void collect_icons(const osd_cycle *cycle, cairo_surface_t **icons);
static void update_previews(osd_cycle *cycle);
static void update_preview(osd_cycle *cycle, size_t index);
static osd_preview *preview_slot(osd_cycle *cycle, size_t index);
static void place_preview(osd_cycle *cycle, size_t index, struct wlr_scene_buffer *node,
                          const struct wlr_surface *surface);
static void clear_previews_from(osd_cycle *cycle, size_t first);
static void destroy_previews(osd_cycle *cycle);
static bool lists_view(const osd_cycle *cycle, const view *v);
static void handle_repaint_idle(void *data);
static void cancel_repaint(osd_cycle *cycle);

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
    cancel_repaint(cycle);
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
    clear_previews_from(cycle, cycle->view.preview ? cycle->view.count : 0);
    if (cycle->view.preview) {
        update_previews(cycle);
    }
    cycle->shown = true;
    wlr_scene_node_set_enabled(&cycle->tree->node, true);
}

void osd_cycle_hide(osd_cycle *cycle)
{
    cancel_repaint(cycle);
    cycle->shown = false;
    clear_previews_from(cycle, 0);
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

void osd_cycle_icon_changed(osd_cycle *cycle, view *v)
{
    if (!cycle->shown || cycle->repaint_idle != NULL || !lists_view(cycle, v)) {
        return;
    }
    cycle->repaint_idle = wl_event_loop_add_idle(cycle->server->loop, handle_repaint_idle, cycle);
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
        utf8_copy(cycle->titles[i], CYCLE_TITLE_MAX, view->items[i].title);
        cycle->view.items[i].title = cycle->titles[i];
        utf8_copy(cycle->app_names[i], CYCLE_TITLE_MAX, view->items[i].app_name);
        cycle->view.items[i].app_name = cycle->app_names[i];
    }
}

static void measure(osd_cycle *cycle)
{
    const compositor_osd_painter *painter = osd_painter_of(cycle->server);
    cycle->layout = painter->layout_cycle(painter->self, &cycle->view);
    cycle->origin = osd_center_origin(cycle->layout.size, &cycle->view.monitor);
}

static void paint_panel(osd_cycle *cycle)
{
    const compositor_osd_painter *painter = osd_painter_of(cycle->server);
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
    double scale = outputs_max_scale(cycle->server->outputs);
    for (size_t i = 0; i < cycle->view.count; i++) {
        view *v = cycle->view.items[i].window;
        icons[i] = v != NULL ? view_icon_for(v, &cycle->layout.icons[i], scale) : NULL;
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
    struct wlr_surface *surface = v != NULL ? view_surface(v) : NULL;
    bool shown = osd_preview_show(preview, surface);
    wlr_scene_node_set_enabled(&node->node, shown);
    if (shown) {
        place_preview(cycle, index, node, surface);
    }
}

static osd_preview *preview_slot(osd_cycle *cycle, size_t index)
{
    if (cycle->previews[index] == NULL) {
        cycle->previews[index] = osd_preview_create(cycle->server, cycle->tree);
    }
    return cycle->previews[index];
}

static void place_preview(osd_cycle *cycle, size_t index, struct wlr_scene_buffer *node,
                          const struct wlr_surface *surface)
{
    wm_size surface_size = {surface->current.width, surface->current.height};
    wm_rect fitted = osd_fit_preview(surface_size, &cycle->layout.previews[index]);
    wlr_scene_buffer_set_dest_size(node, fitted.width, fitted.height);
    wlr_scene_node_set_position(&node->node, cycle->origin.x + fitted.x,
                                cycle->origin.y + fitted.y);
}

static void clear_previews_from(osd_cycle *cycle, size_t first)
{
    for (size_t i = first; i < WM_CYCLE_ITEM_MAX; i++) {
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

static bool lists_view(const osd_cycle *cycle, const view *v)
{
    for (size_t i = 0; i < cycle->view.count; i++) {
        if (cycle->view.items[i].window == v) {
            return true;
        }
    }
    return false;
}

static void handle_repaint_idle(void *data)
{
    osd_cycle *cycle = data;
    cycle->repaint_idle = NULL;
    osd_cycle_repaint(cycle);
}

static void cancel_repaint(osd_cycle *cycle)
{
    if (cycle->repaint_idle != NULL) {
        wl_event_source_remove(cycle->repaint_idle);
        cycle->repaint_idle = NULL;
    }
}
