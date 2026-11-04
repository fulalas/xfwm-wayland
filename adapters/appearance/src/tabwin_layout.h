#ifndef APPEARANCE_TABWIN_LAYOUT_H
#define APPEARANCE_TABWIN_LAYOUT_H

#include "compositor/painter.h"

enum {
    TABWIN_RADIUS = 10,
    TABWIN_BORDER = 1,
};

cycle_layout appearance_tabwin_layout(const wm_cycle_view *view, int label_height);
wm_rect appearance_tabwin_icon_rect(const wm_cycle_view *view, const wm_rect *cell);
wm_rect appearance_tabwin_text_rect(const wm_cycle_view *view, const wm_rect *cell,
                                    int label_height);
wm_rect appearance_tabwin_label_rect(wm_size size, int label_height);
size_t appearance_tabwin_item_count(const wm_cycle_view *view);

#endif
