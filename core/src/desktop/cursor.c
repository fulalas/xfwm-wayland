#include "cursor.h"

#include "desktop_state.h"
#include "present.h"

static wm_cursor_shape cursor_for_part(wm_part part);

void cursor_show_session(wm_desktop *desktop)
{
    const move_session *session = &desktop->session;
    if (session->kind == SESSION_MOVE) {
        present_set_cursor(desktop, WM_CURSOR_MOVE);
        return;
    }
    wm_cursor_shape shape = cursor_for_part(session->handle);
    present_set_cursor(desktop, shape == WM_CURSOR_DEFAULT ? WM_CURSOR_MOVE : shape);
}

void cursor_show_hover(wm_desktop *desktop, const wm_window *window, wm_part part)
{
    if (window == NULL || !window_can_resize(window)) {
        present_set_cursor(desktop, WM_CURSOR_DEFAULT);
        return;
    }
    present_set_cursor(desktop, cursor_for_part(part));
}

static wm_cursor_shape cursor_for_part(wm_part part)
{
    switch (part) {
    case WM_PART_CORNER_TOP_LEFT:
        return WM_CURSOR_RESIZE_TOP_LEFT;
    case WM_PART_CORNER_TOP_RIGHT:
        return WM_CURSOR_RESIZE_TOP_RIGHT;
    case WM_PART_CORNER_BOTTOM_LEFT:
        return WM_CURSOR_RESIZE_BOTTOM_LEFT;
    case WM_PART_CORNER_BOTTOM_RIGHT:
        return WM_CURSOR_RESIZE_BOTTOM_RIGHT;
    case WM_PART_SIDE_TOP:
        return WM_CURSOR_RESIZE_TOP;
    case WM_PART_SIDE_BOTTOM:
        return WM_CURSOR_RESIZE_BOTTOM;
    case WM_PART_SIDE_LEFT:
        return WM_CURSOR_RESIZE_LEFT;
    case WM_PART_SIDE_RIGHT:
        return WM_CURSOR_RESIZE_RIGHT;
    default:
        return WM_CURSOR_DEFAULT;
    }
}
