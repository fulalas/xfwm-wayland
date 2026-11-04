#ifndef COMPOSITOR_OSD_GEOMETRY_H
#define COMPOSITOR_OSD_GEOMETRY_H

#include <stddef.h>

#include "wm/geometry.h"

wm_point osd_clamp_origin(wm_point anchor, wm_size size, const wm_rect *area);
wm_rect osd_item_row(const wm_rect *panel, const wm_rect *item);
wm_point osd_submenu_origin(const wm_rect *row, wm_size size, const wm_rect *area);
wm_point osd_center_origin(wm_size size, const wm_rect *area);
wm_rect osd_fit_preview(wm_size content, const wm_rect *slot);
int osd_item_at(const wm_rect *items, size_t count, wm_point point);

#endif
