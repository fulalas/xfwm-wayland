#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wlr/types/wlr_scene.h>

#include "cairo_buffer.h"
#include "osd_geometry.h"
#include "osd_parts.h"
#include "output.h"

enum {
    MENU_TEXT_MAX = 128,
};

typedef struct menu_panel {
    node_owner owner;
    struct wlr_scene_buffer *node;
    wm_menu_level level;
    menu_layout layout;
    char texts[WM_MENU_ITEM_MAX][MENU_TEXT_MAX];
    wm_point origin;
    bool shown;
} menu_panel;

struct osd_menu {
    compositor *server;
    struct wlr_scene_tree *tree;
    menu_panel main;
    menu_panel sub;
};

static bool panel_init(menu_panel *panel, struct wlr_scene_tree *tree);
static bool panel_holds(const menu_panel *panel, const wm_menu_level *level);
static bool same_items(const menu_panel *panel, const wm_menu_level *level);
static bool same_item(const wm_menu_item *held, const wm_menu_item *item);
static size_t item_count(const wm_menu_level *level);
static void panel_store(menu_panel *panel, const wm_menu_level *level);
static void panel_refresh(const osd_menu *menu, menu_panel *panel);
static void panel_paint(const osd_menu *menu, menu_panel *panel);
static int panel_item_at(const menu_panel *panel, wm_point point);
static wm_point main_origin(const osd_menu *menu, wm_size size);
static wm_point submenu_origin(const osd_menu *menu, wm_size size);
static wm_rect selected_item(const menu_panel *panel);
static wm_rect area_for(const osd_menu *menu, wm_point anchor);
static void panel_hide(menu_panel *panel);

osd_menu *osd_menu_create(compositor *server, struct wlr_scene_tree *parent)
{
    osd_menu *menu = calloc(1, sizeof(*menu));
    if (menu == NULL) {
        return NULL;
    }
    menu->server = server;
    menu->main.owner = (node_owner){NODE_OSD_MENU, menu};
    menu->sub.owner = (node_owner){NODE_OSD_SUBMENU, menu};
    menu->tree = wlr_scene_tree_create(parent);
    if (menu->tree == NULL || !panel_init(&menu->main, menu->tree) ||
        !panel_init(&menu->sub, menu->tree)) {
        osd_menu_destroy(menu);
        return NULL;
    }
    return menu;
}

void osd_menu_destroy(osd_menu *menu)
{
    if (menu->tree != NULL) {
        wlr_scene_node_destroy(&menu->tree->node);
    }
    free(menu);
}

void osd_menu_show(osd_menu *menu, const wm_menu_view *view)
{
    bool main_changed = !panel_holds(&menu->main, &view->main);
    if (main_changed) {
        panel_store(&menu->main, &view->main);
        panel_refresh(menu, &menu->main);
    }
    if (!view->submenu_open) {
        panel_hide(&menu->sub);
        return;
    }
    if (main_changed || !panel_holds(&menu->sub, &view->submenu)) {
        panel_store(&menu->sub, &view->submenu);
        panel_refresh(menu, &menu->sub);
    }
}

void osd_menu_hide(osd_menu *menu)
{
    panel_hide(&menu->main);
    panel_hide(&menu->sub);
}

bool osd_menu_is_shown(const osd_menu *menu)
{
    return menu->main.shown;
}

int osd_menu_item_at(const osd_menu *menu, node_role role, wm_point point)
{
    const menu_panel *panel = role == NODE_OSD_SUBMENU ? &menu->sub : &menu->main;
    if (!panel->shown) {
        return -1;
    }
    return panel_item_at(panel, point);
}

void osd_menu_repaint(osd_menu *menu)
{
    if (menu->main.shown) {
        panel_refresh(menu, &menu->main);
    }
    if (menu->sub.shown) {
        panel_refresh(menu, &menu->sub);
    }
}

static bool panel_init(menu_panel *panel, struct wlr_scene_tree *tree)
{
    panel->node = wlr_scene_buffer_create(tree, NULL);
    if (panel->node == NULL) {
        return false;
    }
    node_owner_attach(&panel->node->node, &panel->owner);
    wlr_scene_node_set_enabled(&panel->node->node, false);
    return true;
}

static bool panel_holds(const menu_panel *panel, const wm_menu_level *level)
{
    const wm_menu_level *held = &panel->level;
    if (!panel->shown || held->selected != level->selected || held->anchor.x != level->anchor.x ||
        held->anchor.y != level->anchor.y || held->count != item_count(level)) {
        return false;
    }
    return same_items(panel, level);
}

static bool same_items(const menu_panel *panel, const wm_menu_level *level)
{
    for (size_t i = 0; i < panel->level.count; i++) {
        if (!same_item(&panel->level.items[i], &level->items[i])) {
            return false;
        }
    }
    return true;
}

static bool same_item(const wm_menu_item *held, const wm_menu_item *item)
{
    const char *text = item->text != NULL ? item->text : "";
    return held->label == item->label && held->sensitive == item->sensitive &&
           held->has_submenu == item->has_submenu && held->mark == item->mark &&
           strcmp(held->text, text) == 0;
}

static size_t item_count(const wm_menu_level *level)
{
    return level->count < WM_MENU_ITEM_MAX ? level->count : WM_MENU_ITEM_MAX;
}

static void panel_store(menu_panel *panel, const wm_menu_level *level)
{
    panel->level = *level;
    size_t count = item_count(level);
    panel->level.count = count;
    for (size_t i = 0; i < count; i++) {
        const char *text = level->items[i].text;
        snprintf(panel->texts[i], MENU_TEXT_MAX, "%s", text != NULL ? text : "");
        panel->level.items[i].text = panel->texts[i];
    }
}

static void panel_refresh(const osd_menu *menu, menu_panel *panel)
{
    const compositor_osd_painter *painter = osd_painter_of(menu->server);
    panel->layout = painter->layout_menu(painter->self, &panel->level);
    panel_paint(menu, panel);
}

static void panel_paint(const osd_menu *menu, menu_panel *panel)
{
    const compositor_osd_painter *painter = osd_painter_of(menu->server);
    wm_size size = panel->layout.size;
    menu_paint_request request = {
        .level = &panel->level,
        .layout = &panel->layout,
        .scale = outputs_max_scale(menu->server->outputs),
    };
    cairo_surface_t *surface = painter->paint_menu(painter->self, &request);
    bool attached = cairo_buffer_attach_sized(panel->node, surface, size);
    panel->origin = panel == &menu->sub ? submenu_origin(menu, size) : main_origin(menu, size);
    wlr_scene_node_set_position(&panel->node->node, panel->origin.x, panel->origin.y);
    wlr_scene_node_set_enabled(&panel->node->node, attached);
    panel->shown = attached;
}

static int panel_item_at(const menu_panel *panel, wm_point point)
{
    wm_point local = {point.x - panel->origin.x, point.y - panel->origin.y};
    int index = osd_item_at(panel->layout.items, panel->layout.count, local);
    if (index < 0 || panel->level.items[index].label == WM_MENU_SEPARATOR) {
        return -1;
    }
    return index;
}

static wm_point main_origin(const osd_menu *menu, wm_size size)
{
    wm_point anchor = menu->main.level.anchor;
    wm_rect area = area_for(menu, anchor);
    return osd_clamp_origin(anchor, size, &area);
}

static wm_point submenu_origin(const osd_menu *menu, wm_size size)
{
    const menu_panel *main = &menu->main;
    wm_rect panel = {main->origin.x, main->origin.y, main->layout.size.width,
                     main->layout.size.height};
    wm_rect item = selected_item(main);
    wm_rect row = osd_item_row(&panel, &item);
    wm_rect area = area_for(menu, main->origin);
    return osd_submenu_origin(&row, size, &area);
}

static wm_rect selected_item(const menu_panel *panel)
{
    int index = panel->level.selected;
    if (index < 0 || (size_t) index >= panel->layout.count) {
        return (wm_rect){0, 0, 0, 0};
    }
    return panel->layout.items[index];
}

static wm_rect area_for(const osd_menu *menu, wm_point anchor)
{
    output *o = outputs_near_point(menu->server->outputs, anchor);
    return o != NULL ? output_full(o) : (wm_rect){anchor.x, anchor.y, 0, 0};
}

static void panel_hide(menu_panel *panel)
{
    panel->shown = false;
    wlr_scene_node_set_enabled(&panel->node->node, false);
}
