#ifndef DESKTOP_FRAME_BUTTONS_H
#define DESKTOP_FRAME_BUTTONS_H

#include <stdbool.h>
#include <stdint.h>

#include "wm/desktop.h"

typedef struct frame_buttons_state {
    bool pressing;
    wm_window *window;
    wm_button button;
    uint32_t mouse_button;
    wm_window *hover_window;
} frame_buttons_state;

void frame_buttons_press(wm_desktop *desktop, wm_window *window, wm_button button,
                         uint32_t mouse_button);
bool frame_buttons_is_pressing(const wm_desktop *desktop);
wm_dispatch frame_buttons_motion(wm_desktop *desktop, const wm_pointer_event *event);
wm_dispatch frame_buttons_button(wm_desktop *desktop, const wm_button_event *event);
void frame_buttons_hover(wm_desktop *desktop, const wm_pointer_event *event);
void frame_buttons_window_gone(wm_desktop *desktop, const wm_window *window);

#endif
