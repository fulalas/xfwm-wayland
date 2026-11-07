#ifndef COMPOSITOR_OSD_PARTS_H
#define COMPOSITOR_OSD_PARTS_H

#include <stdbool.h>

#include "util/node_owner.h"
#include "server/server.h"
#include "shell/view.h"
#include "wm/views.h"

struct wlr_scene_tree;

typedef struct osd_menu osd_menu;
typedef struct osd_cycle osd_cycle;
typedef struct osd_outline osd_outline;
typedef struct osd_size osd_size;
typedef struct osd_find_cursor osd_find_cursor;

osd_menu *osd_menu_create(compositor *server, struct wlr_scene_tree *parent);
void osd_menu_destroy(osd_menu *menu);
void osd_menu_show(osd_menu *menu, const wm_menu_view *view);
void osd_menu_hide(osd_menu *menu);
bool osd_menu_is_shown(const osd_menu *menu);
int osd_menu_item_at(const osd_menu *menu, node_role role, wm_point point);
void osd_menu_repaint(osd_menu *menu);

osd_cycle *osd_cycle_create(compositor *server, struct wlr_scene_tree *parent);
void osd_cycle_destroy(osd_cycle *cycle);
void osd_cycle_show(osd_cycle *cycle, const wm_cycle_view *view);
void osd_cycle_hide(osd_cycle *cycle);
bool osd_cycle_is_shown(const osd_cycle *cycle);
int osd_cycle_item_at(const osd_cycle *cycle, wm_point point);
void osd_cycle_view_committed(osd_cycle *cycle, view *view);
void osd_cycle_icon_changed(osd_cycle *cycle, view *view);
void osd_cycle_view_destroyed(osd_cycle *cycle, view *view);
void osd_cycle_repaint(osd_cycle *cycle);

osd_outline *osd_outline_create(struct wlr_scene_tree *parent);
void osd_outline_destroy(osd_outline *outline);
void osd_outline_show(osd_outline *outline, const wm_rect *rect);
void osd_outline_hide(osd_outline *outline);

osd_size *osd_size_create(compositor *server, struct wlr_scene_tree *parent);
void osd_size_destroy(osd_size *label);
void osd_size_show(osd_size *label, const wm_size_view *view);
void osd_size_hide(osd_size *label);

osd_find_cursor *osd_find_cursor_create(compositor *server, struct wlr_scene_tree *parent);
void osd_find_cursor_destroy(osd_find_cursor *find);
void osd_find_cursor_start(osd_find_cursor *find);

static inline const compositor_osd_painter *osd_painter_of(const compositor *server)
{
    return &server->painter->osd;
}

#endif
