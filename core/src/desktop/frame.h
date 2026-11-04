#ifndef DESKTOP_FRAME_H
#define DESKTOP_FRAME_H

#include <stdbool.h>

#include "wm/desktop.h"

wm_decor_window frame_decor_window(const wm_desktop *desktop, const wm_window *window);
wm_extents frame_extents(const wm_desktop *desktop, const wm_window *window);
wm_rect frame_rect(const wm_desktop *desktop, const wm_window *window);
wm_point frame_center(const wm_desktop *desktop, const wm_window *window);
wm_rect frame_rect_for_client(const wm_desktop *desktop, const wm_window *window,
                              const wm_rect *client);
bool frame_is_shown(const wm_desktop *desktop, const wm_window *window);
int frame_title_height(const wm_desktop *desktop);

#endif
