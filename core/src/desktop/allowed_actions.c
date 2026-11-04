#include "allowed_actions.h"

static uint32_t layer_actions(const wm_window *window);
static uint32_t geometry_actions(const wm_window *window);
static uint32_t workspace_actions(const wm_window *window);

uint32_t allowed_actions_of(const wm_window *window)
{
    uint32_t actions = WM_ALLOW_CLOSE | WM_ALLOW_FULLSCREEN;
    actions |= layer_actions(window);
    actions |= geometry_actions(window);
    actions |= workspace_actions(window);
    if (window_can_hide(window)) {
        actions |= WM_ALLOW_MINIMIZE;
    }
    if (window_has_flag(window, WINDOW_HAS_BORDER)) {
        actions |= WM_ALLOW_SHADE;
    }
    return actions;
}

static uint32_t layer_actions(const wm_window *window)
{
    return window_is_regular_focusable(window) ? WM_ALLOW_ABOVE | WM_ALLOW_BELOW : 0;
}

static uint32_t geometry_actions(const wm_window *window)
{
    uint32_t actions = 0;
    if (window_has_flag(window, WINDOW_HAS_MOVE)) {
        actions |= WM_ALLOW_MOVE;
    }
    if (window_can_resize(window) && !window_is_maximized(window)) {
        actions |= WM_ALLOW_RESIZE;
    }
    if (window_can_maximize(window)) {
        actions |= WM_ALLOW_MAXIMIZE;
    }
    return actions;
}

static uint32_t workspace_actions(const wm_window *window)
{
    return window_has_flag(window, WINDOW_HAS_STICK) ? WM_ALLOW_STICK | WM_ALLOW_CHANGE_WORKSPACE
                                                     : 0;
}
