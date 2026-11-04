#ifndef DESKTOP_EFFECTS_H
#define DESKTOP_EFFECTS_H

#include <stdbool.h>

#include "wm/desktop.h"

wm_opacity_view effects_opacity(const wm_desktop *desktop, const wm_window *window);
bool effects_has_shadow(const wm_desktop *desktop, const wm_window *window);
wm_rect effects_shadow_rect(const wm_desktop *desktop, const wm_window *window);
void effects_increase_opacity(wm_desktop *desktop, wm_window *window);
void effects_decrease_opacity(wm_desktop *desktop, wm_window *window);
bool effects_enabled(const wm_desktop *desktop);

#endif
