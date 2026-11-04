#ifndef APPEARANCE_MENU_MARKS_H
#define APPEARANCE_MENU_MARKS_H

#include <cairo.h>

#include "wm/views.h"

void appearance_menu_mark_draw(cairo_t *cr, wm_menu_mark mark, const wm_rect *box);
void appearance_menu_arrow_draw(cairo_t *cr, const wm_rect *box);

#endif
