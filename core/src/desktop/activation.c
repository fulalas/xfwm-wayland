#include "desktop_state.h"
#include "focus.h"
#include "present.h"
#include "stack.h"
#include "transients.h"
#include "urgency.h"
#include "visibility.h"
#include "workspaces.h"

static bool is_refused(const wm_desktop *desktop, wm_activation_source source);
static bool may_activate(const wm_desktop *desktop, const wm_window *window,
                         wm_activation_source source);
static void activate(wm_desktop *desktop, wm_window *window, wm_activation_source source);
static bool focus_is_on_child(const wm_desktop *desktop, const wm_window *window);
static void reach_workspace(wm_desktop *desktop, wm_window *window, wm_activation_source source);

void wm_request_activate(wm_desktop *desktop, wm_window *window, wm_activation_source source)
{
    if (is_refused(desktop, source) || !may_activate(desktop, window, source)) {
        urgency_set(desktop, window);
    } else {
        activate(desktop, window, source);
    }
    present_flush(desktop);
}

static bool is_refused(const wm_desktop *desktop, wm_activation_source source)
{
    const wm_focus_config *config = &desktop->config.focus;
    if (source == WM_ACTIVATION_PAGER || !config->prevent_focus_stealing) {
        return false;
    }
    return source == WM_ACTIVATION_APPLICATION_UNTRUSTED
           || config->activate_action == WM_ACTIVATE_NONE;
}

static bool may_activate(const wm_desktop *desktop, const wm_window *window,
                         wm_activation_source source)
{
    return source == WM_ACTIVATION_PAGER || visibility_on_current_workspace(desktop, window)
           || desktop->config.focus.activate_action != WM_ACTIVATE_NONE;
}

static void activate(wm_desktop *desktop, wm_window *window, wm_activation_source source)
{
    if (window->was_shown) {
        visibility_clear_show_desktop(desktop);
    }
    reach_workspace(desktop, window, source);
    visibility_unminimize(desktop, transient_root(window));
    stack_raise(desktop, window);
    bool application = source != WM_ACTIVATION_PAGER;
    if (application && focus_is_on_child(desktop, window)) {
        return;
    }
    if (!application || desktop->config.focus.click_to_focus || window_is_dont_focus(window)) {
        focus_set(desktop, window);
    }
}

static void reach_workspace(wm_desktop *desktop, wm_window *window, wm_activation_source source)
{
    int current = desktop->workspaces.current;
    if (visibility_on_current_workspace(desktop, window)) {
        return;
    }
    bool bring = desktop->config.focus.activate_action == WM_ACTIVATE_BRING;
    if (bring && source != WM_ACTIVATION_PAGER) {
        visibility_set_workspace(desktop, window, current);
    } else {
        workspaces_switch_without_focus(desktop, window->workspace, NULL);
    }
}

static bool focus_is_on_child(const wm_desktop *desktop, const wm_window *window)
{
    const wm_window *focused = desktop->focus.focused;
    return focused != NULL && transient_is_descendant_of(focused, window);
}
