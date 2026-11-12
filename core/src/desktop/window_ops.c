#include "window_ops.h"

#include "desktop_state.h"
#include "fill.h"
#include "maximize.h"
#include "placement.h"
#include "stack.h"
#include "states.h"
#include "tile.h"
#include "visibility.h"

void window_ops_close(wm_desktop *desktop, wm_window *window)
{
    window->close_requested = true;
    desktop->ports.commands.close(desktop->ports.commands.self, window->handle);
}

void window_ops_hide(wm_desktop *desktop, wm_window *window)
{
    if (window_can_hide(window)) {
        visibility_minimize(desktop, window);
    }
}

void window_ops_double_click(wm_desktop *desktop, wm_window *window)
{
    switch (desktop->config.mouse.double_click_action) {
    case WM_DOUBLE_CLICK_MAXIMIZE:
        maximize_toggle(desktop, window, WM_STATE_MAXIMIZED);
        break;
    case WM_DOUBLE_CLICK_SHADE:
        states_toggle_shade(desktop, window);
        break;
    case WM_DOUBLE_CLICK_FILL:
        fill_window(desktop, window, FILL_BOTH);
        break;
    case WM_DOUBLE_CLICK_HIDE:
        window_ops_hide(desktop, window);
        break;
    case WM_DOUBLE_CLICK_ABOVE:
        states_toggle_above(desktop, window);
        break;
    default:
        break;
    }
}

void window_ops_show(wm_desktop *desktop, wm_window *window)
{
    if (window->was_shown) {
        visibility_clear_show_desktop(desktop);
    }
    visibility_unminimize(desktop, window);
}

void window_ops_raise_or_lower(wm_desktop *desktop, wm_window *window)
{
    if (stack_is_top_most(desktop, window)) {
        stack_lower(desktop, window);
    } else {
        stack_raise(desktop, window);
    }
}

void window_ops_send_to_workspace(wm_desktop *desktop, wm_window *window, int workspace)
{
    if (workspace >= 0 && workspace < desktop->workspaces.count) {
        stack_raise(desktop, window);
        visibility_set_workspace(desktop, window, workspace);
        window_placed(window);
    }
}

void window_ops_toggle_stick(wm_desktop *desktop, wm_window *window)
{
    if (window_has_flag(window, WINDOW_HAS_STICK)) {
        states_toggle_stick(desktop, window);
    }
}

void window_ops_refit(wm_desktop *desktop, wm_window *window)
{
    if (window_is_fullscreen(window)) {
        states_update_fullscreen_size(desktop, window);
    } else if (window->tile != TILE_NONE) {
        tile_update_size(desktop, window);
    } else if (window_has_state(window, WM_STATE_MAXIMIZED)) {
        maximize_update_size(desktop, window);
    } else if (window_is_constrained(window)) {
        placement_constrain(desktop, window);
    }
    window_mark_dirty(window);
}
