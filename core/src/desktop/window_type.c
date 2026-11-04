#include "window.h"

static wm_window_type resolved_type(const wm_window *window);
static uint32_t type_states(const wm_window *window);

void window_apply_type(wm_window *window)
{
    window->type = resolved_type(window);
    window->states |= type_states(window);
    if (window->type == WM_TYPE_NOTIFICATION) {
        window->states &= ~(uint32_t) (WM_STATE_ABOVE | WM_STATE_BELOW);
    }
}

static wm_window_type resolved_type(const wm_window *window)
{
    if (window->requested_type != WM_TYPE_UNSPECIFIED) {
        return window->requested_type;
    }
    return window->parent != NULL ? WM_TYPE_DIALOG : WM_TYPE_NORMAL;
}

static uint32_t type_states(const wm_window *window)
{
    switch (window->type) {
    case WM_TYPE_DESKTOP:
    case WM_TYPE_DOCK:
        return WM_STATE_SKIP_PAGER | WM_STATE_SKIP_TASKBAR | WM_STATE_STICKY;
    case WM_TYPE_TOOLBAR:
    case WM_TYPE_MENU:
    case WM_TYPE_UTILITY:
    case WM_TYPE_SPLASH:
        return WM_STATE_SKIP_PAGER | WM_STATE_SKIP_TASKBAR;
    case WM_TYPE_NOTIFICATION:
        return WM_STATE_SKIP_TASKBAR;
    case WM_TYPE_DIALOG:
        return window->parent != NULL ? WM_STATE_SKIP_TASKBAR : 0;
    default:
        return 0;
    }
}
