#ifndef DESKTOP_CYCLE_H
#define DESKTOP_CYCLE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "tabwin.h"

typedef struct cycle_session {
    bool active;
    bool shown;
    bool dirty;
    wm_window *items[WM_CYCLE_ITEM_MAX];
    size_t count;
    size_t selected;
    uint32_t modifiers;
    tabwin_grid grid;
    wm_cycle_view view;
} cycle_session;

void cycle_start(wm_desktop *desktop, wm_action action);
bool cycle_is_active(const wm_desktop *desktop);
void cycle_select(wm_desktop *desktop, size_t index);
void cycle_end(wm_desktop *desktop);
void cycle_abort(wm_desktop *desktop);
void cycle_window_gone(wm_desktop *desktop, const wm_window *window);
void cycle_present(wm_desktop *desktop);
void cycle_switch_window(wm_desktop *desktop);
void cycle_switch_application(wm_desktop *desktop);

wm_dispatch cycle_key(wm_desktop *desktop, const wm_key_event *event);
wm_dispatch cycle_key_release(wm_desktop *desktop, const wm_key_event *event);
void cycle_modifiers_changed(wm_desktop *desktop, uint32_t modifiers);
wm_dispatch cycle_motion(wm_desktop *desktop, const wm_pointer_event *event);
wm_dispatch cycle_button(wm_desktop *desktop, const wm_button_event *event);
wm_dispatch cycle_axis(wm_desktop *desktop, const wm_axis_event *event);

#endif
