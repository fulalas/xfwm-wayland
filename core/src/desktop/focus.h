#ifndef DESKTOP_FOCUS_H
#define DESKTOP_FOCUS_H

#include <stdbool.h>

#include "window_list.h"
#include "wm/desktop.h"

typedef struct focus_state {
    wm_window *focused;
    wm_window *delayed;
    wm_window *hovered;
    wm_window *last_raise;
} focus_state;

bool focus_accepts(const wm_desktop *desktop, const wm_window *window);
void focus_set(wm_desktop *desktop, wm_window *window);
void focus_clear(wm_desktop *desktop);
void focus_pass(wm_desktop *desktop, wm_window *window);
void focus_pass_after_hide(wm_desktop *desktop, wm_window *window, const window_list *gone);
void focus_top(wm_desktop *desktop, wm_layer layer);
void focus_new_window(wm_desktop *desktop, wm_window *window);
void focus_and_raise_on_click(wm_desktop *desktop, wm_window *window);
void focus_pointer_entered(wm_desktop *desktop, wm_window *window);
void focus_on_delay_elapsed(wm_desktop *desktop, wm_window *window);
void focus_on_raise_delay_elapsed(wm_desktop *desktop);
void focus_forget(wm_desktop *desktop, wm_window *window);
wm_window *focus_window_at(const wm_desktop *desktop, wm_point point, const window_list *exclude);

#endif
