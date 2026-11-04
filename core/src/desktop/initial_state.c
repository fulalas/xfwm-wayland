#include "initial_state.h"

#include "desktop_state.h"
#include "focus.h"
#include "maximize.h"
#include "stack.h"
#include "states.h"
#include "visibility.h"

static void apply_maximize(wm_desktop *desktop, wm_window *window);
static void apply_fullscreen(wm_desktop *desktop, wm_window *window);
static void apply_stick_and_shade(wm_desktop *desktop, wm_window *window);
static void remember_workspace_focus(wm_desktop *desktop, wm_window *window);

void initial_state_apply(wm_desktop *desktop, wm_window *window)
{
    apply_maximize(desktop, window);
    apply_fullscreen(desktop, window);
    stack_update_layers(desktop);
    apply_stick_and_shade(desktop, window);
}

void initial_state_show(wm_desktop *desktop, wm_window *window)
{
    if (window_is_minimized(window)) {
        stack_raise(desktop, window);
        return;
    }
    if (visibility_on_current_workspace(desktop, window)) {
        focus_new_window(desktop, window);
        return;
    }
    stack_raise(desktop, window);
    remember_workspace_focus(desktop, window);
}

static void apply_maximize(wm_desktop *desktop, wm_window *window)
{
    uint32_t mode = window->states & WM_STATE_MAXIMIZED;
    if (mode == 0 || !window_allows_maximize(window)) {
        window->states &= ~(uint32_t) WM_STATE_MAXIMIZED;
        return;
    }
    window->states &= ~(uint32_t) WM_STATE_MAXIMIZED;
    bool restore = window->restore_size_pos;
    window->restore_size_pos = false;
    if (!maximize_toggle_in_place(desktop, window, mode)) {
        window->restore_size_pos = restore;
    }
}

static void apply_fullscreen(wm_desktop *desktop, wm_window *window)
{
    if (!window_is_fullscreen(window)) {
        return;
    }
    window->states &= ~(uint32_t) WM_STATE_FULLSCREEN;
    states_enter_fullscreen(desktop, window);
}

static void apply_stick_and_shade(wm_desktop *desktop, wm_window *window)
{
    if (window_is_sticky(window) && window_has_flag(window, WINDOW_HAS_STICK)) {
        states_stick(desktop, window);
    }
    if (window_is_shaded(window)) {
        window->states &= ~(uint32_t) WM_STATE_SHADED;
        states_shade(desktop, window);
    }
}

static void remember_workspace_focus(wm_desktop *desktop, wm_window *window)
{
    if (!focus_accepts(desktop, window) || window_is_dont_focus(window)) {
        return;
    }
    for (size_t i = 0; i < desktop->stack.count; i++) {
        if (desktop->stack.items[i]->workspace == window->workspace) {
            desktop->stack.items[i]->workspace_focus = false;
        }
    }
    window->workspace_focus = true;
}
