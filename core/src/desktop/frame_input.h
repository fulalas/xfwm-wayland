#ifndef DESKTOP_FRAME_INPUT_H
#define DESKTOP_FRAME_INPUT_H

#include <stdbool.h>

#include "wm/desktop.h"

wm_dispatch frame_input_press(wm_desktop *desktop, const wm_button_event *event);
wm_dispatch frame_input_axis(wm_desktop *desktop, const wm_axis_event *event);
wm_part frame_input_part_at(const wm_window *window, wm_point point);
wm_point frame_input_menu_anchor(const wm_window *window);
bool frame_input_is_easy_click(const wm_desktop *desktop, uint32_t modifiers);

#endif
