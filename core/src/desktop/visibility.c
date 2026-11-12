#include "visibility.h"

#include "desktop_state.h"
#include "focus.h"
#include "moveresize.h"
#include "transients.h"
#include "urgency.h"

static void show_single(wm_desktop *desktop, wm_window *window);
typedef struct withdraw_kind {
    bool (*keeps_shown)(const wm_window *owner, const wm_window *window);
    void (*withdraw)(wm_desktop *desktop, wm_window *window);
} withdraw_kind;

static void withdraw_group(wm_desktop *desktop, wm_window *window, const withdraw_kind *kind);
static bool keeps_shown_on_hide(const wm_window *owner, const wm_window *window);
static bool keeps_shown_on_minimize(const wm_window *owner, const wm_window *window);
static void hide_single(wm_desktop *desktop, wm_window *window);
static void minimize_single(wm_desktop *desktop, wm_window *window);

static const withdraw_kind HIDE = {keeps_shown_on_hide, hide_single};
static const withdraw_kind MINIMIZE = {keeps_shown_on_minimize, minimize_single};
static void move_group_to(wm_desktop *desktop, wm_window *window, int workspace);
static void move_single_to(wm_desktop *desktop, wm_window *window, int workspace);
static void hide_all_for_desktop(wm_desktop *desktop);
static void show_all_after_desktop(wm_desktop *desktop);

bool visibility_on_current_workspace(const wm_desktop *desktop, const wm_window *window)
{
    return window->workspace == desktop->workspaces.current || window_is_sticky(window);
}

void visibility_show(wm_desktop *desktop, wm_window *window)
{
    window_list group = transient_list(desktop, window);
    for (size_t i = group.count; i > 0; i--) {
        if (!window_is_sticky(group.items[i - 1])) {
            group.items[i - 1]->workspace = window->workspace;
        }
        show_single(desktop, group.items[i - 1]);
    }
    window_list_free(&group);
}

void visibility_unminimize(wm_desktop *desktop, wm_window *window)
{
    window_list group = transient_list(desktop, window);
    for (size_t i = 0; i < group.count; i++) {
        if (window_is_minimized(group.items[i])) {
            group.items[i]->states &= ~(uint32_t) WM_STATE_MINIMIZED;
            window_mark_dirty(group.items[i]);
        }
    }
    window_list_free(&group);
    visibility_show(desktop, window);
}

void visibility_hide(wm_desktop *desktop, wm_window *window)
{
    withdraw_group(desktop, window, &HIDE);
}

void visibility_minimize(wm_desktop *desktop, wm_window *window)
{
    withdraw_group(desktop, window, &MINIMIZE);
}

void visibility_minimize_others(wm_desktop *desktop, wm_window *window)
{
    window_list *stack = &desktop->stack;
    window_list targets = {0};
    for (size_t i = 0; i < stack->count; i++) {
        wm_window *other = stack->items[i];
        if (other != window && window_can_hide(other) && other->parent == NULL
            && !transient_is_for(window, other) && other->workspace == window->workspace) {
            window_list_append(&targets, other);
        }
    }
    for (size_t i = 0; i < targets.count; i++) {
        visibility_minimize(desktop, targets.items[i]);
    }
    window_list_free(&targets);
}

void visibility_set_workspace(wm_desktop *desktop, wm_window *window, int workspace)
{
    if (workspace < 0 || workspace >= desktop->workspaces.count) {
        return;
    }
    move_group_to(desktop, window, workspace);
}

void visibility_assign_workspace(wm_desktop *desktop, wm_window *window, int workspace)
{
    window_list group = transient_list(desktop, window);
    for (size_t i = 0; i < group.count; i++) {
        group.items[i]->workspace = workspace;
        window_mark_dirty(group.items[i]);
    }
    window_list_free(&group);
}

void visibility_clear_show_desktop(wm_desktop *desktop)
{
    if (!desktop->workspaces.show_desktop) {
        return;
    }
    for (size_t i = 0; i < desktop->stack.count; i++) {
        desktop->stack.items[i]->was_shown = false;
    }
    desktop->workspaces.show_desktop = false;
}

void visibility_toggle_show_desktop(wm_desktop *desktop)
{
    desktop->workspaces.show_desktop = !desktop->workspaces.show_desktop;
    focus_clear(desktop);
    if (desktop->workspaces.show_desktop) {
        hide_all_for_desktop(desktop);
        focus_top(desktop, WM_LAYER_DESKTOP);
    } else {
        show_all_after_desktop(desktop);
        focus_top(desktop, WM_LAYER_FULLSCREEN);
    }
}

static void show_single(wm_desktop *desktop, wm_window *window)
{
    if (window->visible || window_is_minimized(window)
        || !visibility_on_current_workspace(desktop, window)) {
        return;
    }
    window->visible = true;
    window_mark_dirty(window);
    urgency_refresh(desktop, window);
}

static void withdraw_group(wm_desktop *desktop, wm_window *window, const withdraw_kind *kind)
{
    window_list group = transient_list(desktop, window);
    window_list gone = {0};
    for (size_t i = 0; i < group.count; i++) {
        if (!kind->keeps_shown(window, group.items[i])) {
            kind->withdraw(desktop, group.items[i]);
            window_list_append(&gone, group.items[i]);
        }
    }
    focus_pass_after_hide(desktop, window, &gone);
    window_list_free(&gone);
    window_list_free(&group);
}

static bool keeps_shown_on_hide(const wm_window *owner, const wm_window *window)
{
    (void) owner;
    return window_is_sticky(window);
}

static bool keeps_shown_on_minimize(const wm_window *owner, const wm_window *window)
{
    return owner->layer < window->layer;
}

static void hide_single(wm_desktop *desktop, wm_window *window)
{
    moveresize_window_gone(desktop, window);
    if (window->visible) {
        window->visible = false;
        urgency_refresh(desktop, window);
    }
    window_mark_dirty(window);
}

static void minimize_single(wm_desktop *desktop, wm_window *window)
{
    hide_single(desktop, window);
    window->states |= WM_STATE_MINIMIZED;
    if (!desktop->workspaces.show_desktop && !desktop->config.cycle.minimized) {
        window_list_move_to_back(&desktop->mru, window);
    }
}

static void move_group_to(wm_desktop *desktop, wm_window *window, int workspace)
{
    window_list group = transient_list(desktop, window);
    for (size_t i = 0; i < group.count; i++) {
        if (group.items[i]->workspace != workspace) {
            move_single_to(desktop, group.items[i], workspace);
        }
    }
    window_list_free(&group);
}

static void move_single_to(wm_desktop *desktop, wm_window *window, int workspace)
{
    int previous = window->workspace;
    window->workspace = workspace;
    window_mark_dirty(window);
    if (window_is_minimized(window)) {
        return;
    }
    if (previous == desktop->workspaces.current) {
        visibility_hide(desktop, window);
    }
    if (window_is_sticky(window) || workspace == desktop->workspaces.current) {
        visibility_show(desktop, window);
    }
}

static void hide_all_for_desktop(wm_desktop *desktop)
{
    window_list targets = {0};
    for (size_t i = 0; i < desktop->stack.count; i++) {
        wm_window *window = desktop->stack.items[i];
        if (window_is_regular_focusable(window) && !window_is_minimized(window)
            && !window_has_state(window, WM_STATE_SKIP_TASKBAR)) {
            window->was_shown = true;
            window_list_append(&targets, window);
        }
    }
    for (size_t i = 0; i < targets.count; i++) {
        visibility_minimize(desktop, targets.items[i]);
    }
    window_list_free(&targets);
}

static void show_all_after_desktop(wm_desktop *desktop)
{
    window_list targets = {0};
    for (size_t i = desktop->stack.count; i > 0; i--) {
        wm_window *window = desktop->stack.items[i - 1];
        if (window->was_shown) {
            window_list_append(&targets, window);
        }
        window->was_shown = false;
    }
    for (size_t i = 0; i < targets.count; i++) {
        visibility_unminimize(desktop, targets.items[i]);
    }
    window_list_free(&targets);
}
