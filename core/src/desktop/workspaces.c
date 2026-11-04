#include "workspaces.h"

#include "desktop_state.h"
#include "focus.h"
#include "monitors.h"
#include "present.h"
#include "stack.h"
#include "visibility.h"

static int resolve_target(const wm_desktop *desktop, int workspace);
static bool apply_switch(wm_desktop *desktop, int workspace, wm_window *carried);
static void show_new_workspace(wm_desktop *desktop);
static void hide_old_workspace(wm_desktop *desktop, wm_window *previous);
static wm_window *workspace_focus_candidate(wm_desktop *desktop, wm_window *previous);
static void focus_after_switch(wm_desktop *desktop, wm_window *new_focus, wm_window *carried);
static int target_column(const wm_desktop *desktop, int delta);
static int wrapped(int value, int limit);
static void store_count(wm_desktop *desktop, int count);
static void assign_new_ids(workspace_state *state, int from, int to);
static void remove_id(workspace_state *state, int position);
static void insert_id(workspace_state *state, int position);

void workspaces_switch(wm_desktop *desktop, int workspace, wm_window *carried)
{
    wm_window *focused = desktop->focus.focused;
    wm_window *new_focus = focused != NULL && focused == carried ? carried : NULL;
    wm_window *previous = focused != NULL && window_is_regular_focusable(focused) ? focused : NULL;
    if (!apply_switch(desktop, workspace, carried)) {
        return;
    }
    hide_old_workspace(desktop, previous);
    wm_window *candidate = workspace_focus_candidate(desktop, previous);
    focus_after_switch(desktop, new_focus ? new_focus : candidate, carried);
}

void workspaces_switch_without_focus(wm_desktop *desktop, int workspace, wm_window *carried)
{
    wm_window *focused = desktop->focus.focused;
    wm_window *previous = focused != NULL && window_is_regular_focusable(focused) ? focused : NULL;
    if (apply_switch(desktop, workspace, carried)) {
        hide_old_workspace(desktop, previous);
        workspace_focus_candidate(desktop, previous);
    }
}

void workspaces_move(wm_desktop *desktop, int column_delta, wm_window *carried)
{
    int current = desktop->workspaces.current;
    int target = target_column(desktop, column_delta);
    if (target != current) {
        workspaces_switch(desktop, target, carried);
    }
}

bool workspaces_move_changed(wm_desktop *desktop, int column_delta, wm_window *carried)
{
    int before = desktop->workspaces.current;
    workspaces_move(desktop, column_delta, carried);
    return desktop->workspaces.current != before;
}

void workspaces_set_count(wm_desktop *desktop, int count)
{
    int clamped = wm_clamp(count, 1, WM_WORKSPACE_MAX);
    workspace_state *state = &desktop->workspaces;
    if (clamped == state->count) {
        return;
    }
    assign_new_ids(state, state->count, clamped);
    state->count = clamped;
    for (size_t i = 0; i < desktop->stack.count; i++) {
        wm_window *window = desktop->stack.items[i];
        if (window->workspace > clamped - 1) {
            visibility_set_workspace(desktop, window, clamped - 1);
        }
    }
    if (state->current > clamped - 1) {
        workspaces_switch(desktop, clamped - 1, NULL);
    }
    present_mark_workspaces(desktop);
}

void workspaces_change_count(wm_desktop *desktop, int count)
{
    int before = desktop->workspaces.count;
    workspaces_set_count(desktop, count);
    if (desktop->workspaces.count != before) {
        store_count(desktop, desktop->workspaces.count);
    }
}

void workspaces_insert(wm_desktop *desktop, int position)
{
    int count = desktop->workspaces.count;
    workspaces_change_count(desktop, count + 1);
    if (position > count || desktop->workspaces.count == count) {
        return;
    }
    insert_id(&desktop->workspaces, position);
    for (size_t i = 0; i < desktop->stack.count; i++) {
        wm_window *window = desktop->stack.items[i];
        if (window->workspace >= position) {
            visibility_set_workspace(desktop, window, window->workspace + 1);
        }
    }
}

void workspaces_delete(wm_desktop *desktop, int position)
{
    int count = desktop->workspaces.count;
    if (count <= 1 || position < 0 || position >= count) {
        return;
    }
    for (size_t i = 0; i < desktop->stack.count; i++) {
        wm_window *window = desktop->stack.items[i];
        if (window->workspace > position) {
            visibility_set_workspace(desktop, window, window->workspace - 1);
        }
    }
    remove_id(&desktop->workspaces, position);
    workspaces_change_count(desktop, count - 1);
}

wm_workspaces_view workspaces_view(const wm_desktop *desktop)
{
    wm_workspaces_view view = {0};
    view.count = desktop->workspaces.count;
    view.current = desktop->workspaces.current;
    for (int i = 0; i < view.count; i++) {
        const char *name = desktop->config.workspaces.names[i];
        view.names[i] = name[0] != '\0' ? name : NULL;
        view.ids[i] = desktop->workspaces.ids[i];
    }
    return view;
}

static int resolve_target(const wm_desktop *desktop, int workspace)
{
    const workspace_state *state = &desktop->workspaces;
    if (workspace == state->current && desktop->config.workspaces.toggle) {
        workspace = state->previous;
    }
    if (!desktop->config.workspaces.wrap_cycle) {
        return workspace;
    }
    if (workspace > state->count - 1) {
        return 0;
    }
    return workspace < 0 ? state->count - 1 : workspace;
}

static bool apply_switch(wm_desktop *desktop, int workspace, wm_window *carried)
{
    workspace_state *state = &desktop->workspaces;
    int target = resolve_target(desktop, workspace);
    if (target == state->current || target < 0 || target > state->count - 1) {
        return false;
    }
    state->previous = state->current;
    state->current = target;
    if (carried != NULL) {
        visibility_assign_workspace(desktop, carried, target);
    }
    show_new_workspace(desktop);
    present_mark_workspaces(desktop);
    return true;
}

static void show_new_workspace(wm_desktop *desktop)
{
    window_list *stack = &desktop->stack;
    int current = desktop->workspaces.current;
    for (size_t i = stack->count; i > 0; i--) {
        wm_window *window = stack->items[i - 1];
        if (window_is_sticky(window)) {
            window->workspace = current;
            window_mark_dirty(window);
        }
        if (window->workspace == current && !window->visible && !window_is_minimized(window)) {
            visibility_show(desktop, window);
        }
    }
}

static void hide_old_workspace(wm_desktop *desktop, wm_window *previous)
{
    window_list *stack = &desktop->stack;
    int current = desktop->workspaces.current;
    for (size_t i = 0; i < stack->count; i++) {
        wm_window *window = stack->items[i];
        if (window->workspace == current) {
            continue;
        }
        if (window == previous) {
            previous->workspace_focus = true;
            focus_clear(desktop);
        }
        if (window->visible && !window_is_sticky(window)) {
            visibility_hide(desktop, window);
        }
    }
}

static wm_window *workspace_focus_candidate(wm_desktop *desktop, wm_window *previous)
{
    wm_window *candidate = NULL;
    window_list *stack = &desktop->stack;
    for (size_t i = stack->count; i > 0; i--) {
        wm_window *window = stack->items[i - 1];
        bool remembered = window_is_sticky(window) ? window == previous : window->workspace_focus;
        if (candidate == NULL && remembered && window->workspace == desktop->workspaces.current
            && !window_is_minimized(window)) {
            candidate = window;
        }
        if (visibility_on_current_workspace(desktop, window)) {
            window->workspace_focus = false;
        }
    }
    return candidate;
}

static void focus_after_switch(wm_desktop *desktop, wm_window *new_focus, wm_window *carried)
{
    if (!desktop->config.focus.click_to_focus && carried == NULL) {
        wm_window *under = focus_window_at(desktop, monitors_pointer(desktop), NULL);
        new_focus = under ? under : new_focus;
    }
    if (new_focus == NULL) {
        focus_top(desktop, WM_LAYER_FULLSCREEN);
        return;
    }
    const wm_focus_config *config = &desktop->config.focus;
    if (config->click_to_focus && config->raise_on_click && !config->raise_on_focus
        && !stack_is_top_most(desktop, new_focus)) {
        stack_raise(desktop, new_focus);
    }
    focus_set(desktop, new_focus);
}

static int target_column(const wm_desktop *desktop, int delta)
{
    int limit = desktop->workspaces.count;
    if (delta >= limit) {
        delta = limit - 1;
    }
    int value = desktop->workspaces.current + delta;
    if (desktop->config.workspaces.wrap_layout) {
        return wrapped(value, limit);
    }
    return wm_clamp(value, 0, limit - 1);
}

static int wrapped(int value, int limit)
{
    if (value >= limit) {
        return value % limit;
    }
    return value < 0 ? (value + limit) % limit : value;
}

static void store_count(wm_desktop *desktop, int count)
{
    desktop->ports.commands.store_workspace_count(desktop->ports.commands.self, count);
}

static void assign_new_ids(workspace_state *state, int from, int to)
{
    for (int i = from; i < to; i++) {
        state->ids[i] = state->next_id++;
    }
}

static void remove_id(workspace_state *state, int position)
{
    for (int i = position; i < state->count - 1; i++) {
        state->ids[i] = state->ids[i + 1];
    }
}

static void insert_id(workspace_state *state, int position)
{
    unsigned appended = state->ids[state->count - 1];
    for (int i = state->count - 1; i > position; i--) {
        state->ids[i] = state->ids[i - 1];
    }
    state->ids[position] = appended;
}
