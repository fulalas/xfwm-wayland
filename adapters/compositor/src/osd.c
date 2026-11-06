#include <stdlib.h>
#include <wlr/types/wlr_scene.h>

#include "osd.h"
#include "osd_parts.h"
#include "scene_layers.h"

struct osd {
    compositor *server;
    osd_menu *menu;
    osd_cycle *cycle;
    osd_outline *outline;
    osd_size *size;
    osd_find_cursor *find_cursor;
};

osd *osd_create(compositor *server)
{
    osd *o = calloc(1, sizeof(*o));
    if (o == NULL) {
        return NULL;
    }
    o->server = server;
    struct wlr_scene_tree *parent = scene_layers_tree(server->layers, SCENE_LAYER_OSD);
    o->outline = osd_outline_create(parent);
    o->size = osd_size_create(server, parent);
    o->cycle = osd_cycle_create(server, parent);
    o->menu = osd_menu_create(server, parent);
    o->find_cursor = osd_find_cursor_create(server, parent);
    if (o->outline == NULL || o->size == NULL || o->cycle == NULL || o->menu == NULL ||
        o->find_cursor == NULL) {
        osd_destroy(o);
        return NULL;
    }
    return o;
}

void osd_destroy(osd *o)
{
    if (o->menu != NULL) {
        osd_menu_destroy(o->menu);
    }
    if (o->cycle != NULL) {
        osd_cycle_destroy(o->cycle);
    }
    if (o->outline != NULL) {
        osd_outline_destroy(o->outline);
    }
    if (o->size != NULL) {
        osd_size_destroy(o->size);
    }
    if (o->find_cursor != NULL) {
        osd_find_cursor_destroy(o->find_cursor);
    }
    free(o);
}

void osd_show_menu(osd *o, const wm_menu_view *view)
{
    osd_menu_show(o->menu, view);
}

void osd_hide_menu(osd *o)
{
    osd_menu_hide(o->menu);
}

void osd_show_cycle(osd *o, const wm_cycle_view *view)
{
    osd_cycle_show(o->cycle, view);
}

void osd_hide_cycle(osd *o)
{
    osd_cycle_hide(o->cycle);
}

void osd_show_outline(osd *o, const wm_rect *rect)
{
    osd_outline_show(o->outline, rect);
}

void osd_hide_outline(osd *o)
{
    osd_outline_hide(o->outline);
}

void osd_show_size(osd *o, const wm_size_view *view)
{
    osd_size_show(o->size, view);
}

void osd_hide_size(osd *o)
{
    osd_size_hide(o->size);
}

void osd_show_find_cursor(osd *o)
{
    osd_find_cursor_start(o->find_cursor);
}

bool osd_is_modal(const osd *o)
{
    return osd_menu_is_shown(o->menu) || osd_cycle_is_shown(o->cycle);
}

wm_pointer_target osd_target_at(const osd *o, node_role role, wm_point point)
{
    switch (role) {
    case NODE_OSD_MENU:
        return (wm_pointer_target){WM_TARGET_MENU, NULL, osd_menu_item_at(o->menu, role, point)};
    case NODE_OSD_SUBMENU:
        return (wm_pointer_target){WM_TARGET_SUBMENU, NULL,
                                   osd_menu_item_at(o->menu, role, point)};
    case NODE_OSD_CYCLE:
        return (wm_pointer_target){WM_TARGET_CYCLE, NULL, osd_cycle_item_at(o->cycle, point)};
    default:
        break;
    }
    return (wm_pointer_target){WM_TARGET_OTHER, NULL, -1};
}

void osd_view_icon_changed(osd *o, view *v)
{
    if (o != NULL && o->cycle != NULL) {
        osd_cycle_icon_changed(o->cycle, v);
    }
}

void osd_view_committed(osd *o, view *v)
{
    if (o != NULL) {
        osd_cycle_view_committed(o->cycle, v);
    }
}

void osd_view_destroyed(osd *o, view *v)
{
    if (o != NULL) {
        osd_cycle_view_destroyed(o->cycle, v);
    }
}

void osd_repaint(osd *o)
{
    if (o != NULL) {
        osd_menu_repaint(o->menu);
        osd_cycle_repaint(o->cycle);
    }
}
