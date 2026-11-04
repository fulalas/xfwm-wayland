#ifndef COMPOSITOR_OSD_H
#define COMPOSITOR_OSD_H

#include <stdbool.h>

#include "node_owner.h"
#include "server.h"
#include "view.h"
#include "wm/desktop.h"

osd *osd_create(compositor *server);
void osd_destroy(osd *osd);
void osd_show_menu(osd *osd, const wm_menu_view *view);
void osd_hide_menu(osd *osd);
void osd_show_cycle(osd *osd, const wm_cycle_view *view);
void osd_hide_cycle(osd *osd);
void osd_show_outline(osd *osd, const wm_rect *rect);
void osd_hide_outline(osd *osd);
void osd_show_find_cursor(osd *osd);
bool osd_is_modal(const osd *osd);
wm_pointer_target osd_target_at(const osd *osd, node_role role, wm_point point);
void osd_view_committed(osd *osd, view *view);
void osd_view_destroyed(osd *osd, view *view);
void osd_repaint(osd *osd);

#endif
