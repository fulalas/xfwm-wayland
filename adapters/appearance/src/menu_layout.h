#ifndef APPEARANCE_MENU_LAYOUT_H
#define APPEARANCE_MENU_LAYOUT_H

#include "compositor/painter.h"
#include "text.h"

enum {
    MENU_BORDER = 1,
    MENU_ITEM_PADDING_X = 6,
    MENU_TOGGLE_SIZE = 16,
    MENU_ARROW_SIZE = 16,
};

typedef struct menu_columns {
    int toggle_width;
    int arrow_width;
} menu_columns;

menu_layout appearance_menu_layout(const text_font *font, const wm_menu_level *level);
menu_columns appearance_menu_columns(const wm_menu_level *level);
wm_rect appearance_menu_label_rect(const menu_columns *columns, const wm_rect *item);
size_t appearance_menu_item_count(const wm_menu_level *level);
bool appearance_menu_is_separator(const wm_menu_level *level, size_t index);

#endif
