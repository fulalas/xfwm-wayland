#ifndef DESKTOP_TRANSIENTS_H
#define DESKTOP_TRANSIENTS_H

#include <stdbool.h>

#include "window_list.h"
#include "wm/desktop.h"

bool transient_is_for(const wm_window *transient, const wm_window *parent);
bool transient_is_descendant_of(const wm_window *window, const wm_window *ancestor);
bool transient_is_modal(const wm_window *window);
bool transient_is_modal_for(const wm_window *modal, const wm_window *parent);
bool transient_same_application(const wm_window *a, const wm_window *b);
bool transient_parent_allowed(const wm_window *window, const wm_window *parent);
wm_window *transient_modal_for(const wm_desktop *desktop, const wm_window *window);
wm_window *transient_root(wm_window *window);
window_list transient_list(const wm_desktop *desktop, wm_window *window);
void transient_detach_all(wm_desktop *desktop, const wm_window *window);

#endif
