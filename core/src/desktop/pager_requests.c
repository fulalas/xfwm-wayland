#include "desktop_state.h"
#include "highlight.h"
#include "monitor_move.h"

void wm_request_send_to_monitor(wm_desktop *desktop, wm_window *window, size_t monitor)
{
    monitor_move_to(desktop, window, monitor);
    present_flush(desktop);
}

void wm_request_keyboard_move(wm_desktop *desktop, wm_window *window)
{
    if (window->visible) {
        moveresize_begin_key_move(desktop, window);
    }
    present_flush(desktop);
}

void wm_request_keyboard_resize(wm_desktop *desktop, wm_window *window)
{
    if (window->visible) {
        moveresize_begin_key_resize(desktop, window);
    }
    present_flush(desktop);
}

void wm_request_highlight(wm_desktop *desktop, wm_window *window)
{
    highlight_begin(desktop, window);
    present_flush(desktop);
}

void wm_request_unhighlight(wm_desktop *desktop)
{
    highlight_end(desktop, desktop->highlighted);
    present_flush(desktop);
}
