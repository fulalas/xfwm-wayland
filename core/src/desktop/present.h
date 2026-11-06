#ifndef DESKTOP_PRESENT_H
#define DESKTOP_PRESENT_H

#include <stdbool.h>

#include "wm/desktop.h"

typedef struct present_state {
    bool stacking_dirty;
    bool workspaces_dirty;
    bool focus_reported;
    void *reported_focus;
    bool outline_dirty;
    bool outline_visible;
    wm_rect outline;
    bool size_visible;
    wm_size_view size;
    bool cursor_reported;
    wm_cursor_shape reported_cursor;
    wm_cursor_shape cursor;
} present_state;

void present_flush(wm_desktop *desktop);
void present_show_outline(wm_desktop *desktop, const wm_rect *rect);
void present_hide_outline(wm_desktop *desktop);
void present_set_cursor(wm_desktop *desktop, wm_cursor_shape shape);
void present_request_configure(wm_window *window);
void present_mark_workspaces(wm_desktop *desktop);

#endif
