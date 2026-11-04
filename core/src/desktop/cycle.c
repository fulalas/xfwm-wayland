#include "cycle.h"

#include "cycle_list.h"
#include "desktop_state.h"
#include "focus.h"
#include "frame.h"
#include "present.h"
#include "stack.h"
#include "states.h"
#include "transients.h"
#include "visibility.h"
#include "workspaces.h"

static wm_window *cycle_origin(const wm_desktop *desktop);
static void open_session(wm_desktop *desktop, size_t selected, uint32_t modifiers);
static void highlight_selected(wm_desktop *desktop);
static void close_session(wm_desktop *desktop);
static void focus_and_raise(wm_desktop *desktop, wm_window *window);
static void build_view(wm_desktop *desktop);
static void remove_item(cycle_session *cycle, size_t index);
static wm_window *previous_matching(wm_desktop *desktop, unsigned mask);
static void cycle_activate(wm_desktop *desktop, wm_window *window);

void cycle_start(wm_desktop *desktop, wm_action action)
{
    cycle_session *cycle = &desktop->cycle;
    wm_window *origin = cycle_origin(desktop);
    if (origin == NULL || cycle->active) {
        return;
    }
    cycle->count = cycle_list_build(desktop, origin, cycle->items);
    if (cycle->count == 0) {
        return;
    }
    bool reverse = action == WM_ACTION_CYCLE_REVERSE_WINDOWS;
    size_t selected = reverse ? cycle->count - 1 : 1;
    if (selected >= cycle->count) {
        selected = 0;
    }
    uint32_t modifiers = keybindings_modifiers_of(&desktop->keys, action);
    if (modifiers == 0) {
        cycle_activate(desktop, cycle->items[selected]);
        return;
    }
    open_session(desktop, selected, modifiers);
}

bool cycle_is_active(const wm_desktop *desktop)
{
    return desktop->cycle.active;
}

void cycle_select(wm_desktop *desktop, size_t index)
{
    cycle_session *cycle = &desktop->cycle;
    if (index >= cycle->count || index == cycle->selected) {
        return;
    }
    cycle->selected = index;
    highlight_selected(desktop);
}

void cycle_end(wm_desktop *desktop)
{
    cycle_session *cycle = &desktop->cycle;
    wm_window *selected = cycle->count > 0 ? cycle->items[cycle->selected] : NULL;
    close_session(desktop);
    if (selected != NULL) {
        cycle_activate(desktop, selected);
    }
}

void cycle_abort(wm_desktop *desktop)
{
    desktop->cycle.selected = 0;
    cycle_end(desktop);
}

void cycle_window_gone(wm_desktop *desktop, const wm_window *window)
{
    cycle_session *cycle = &desktop->cycle;
    for (size_t i = 0; cycle->active && i < cycle->count; i++) {
        if (cycle->items[i] == window) {
            remove_item(cycle, i);
            break;
        }
    }
    if (cycle->active && cycle->count == 0) {
        close_session(desktop);
    } else if (cycle->active) {
        highlight_selected(desktop);
    }
}

void cycle_present(wm_desktop *desktop)
{
    cycle_session *cycle = &desktop->cycle;
    const wm_presenter *presenter = &desktop->ports.presenter;
    if (cycle->active && cycle->dirty) {
        build_view(desktop);
        presenter->cycle_shown(presenter->self, &cycle->view);
        cycle->shown = true;
    } else if (!cycle->active && cycle->shown) {
        presenter->cycle_hidden(presenter->self);
        cycle->shown = false;
    }
    cycle->dirty = false;
}

void cycle_switch_window(wm_desktop *desktop)
{
    wm_window *found = previous_matching(desktop, SEARCH_SAME_APPLICATION);
    if (found != NULL) {
        focus_and_raise(desktop, found);
    }
}

void cycle_switch_application(wm_desktop *desktop)
{
    wm_window *found = previous_matching(desktop, SEARCH_DIFFERENT_APPLICATION
                                                      | SEARCH_NORMAL_ONLY);
    if (found != NULL) {
        focus_and_raise(desktop, found);
    }
}

static wm_window *cycle_origin(const wm_desktop *desktop)
{
    if (desktop->focus.focused != NULL) {
        return desktop->focus.focused;
    }
    const window_list *mru = &desktop->mru;
    return mru->count > 0 ? mru->items[mru->count - 1] : NULL;
}

static void open_session(wm_desktop *desktop, size_t selected, uint32_t modifiers)
{
    cycle_session *cycle = &desktop->cycle;
    cycle->active = true;
    cycle->selected = selected;
    cycle->modifiers = modifiers;
    cycle->grid = tabwin_compute(desktop, cycle->count);
    highlight_selected(desktop);
}

static void highlight_selected(wm_desktop *desktop)
{
    cycle_session *cycle = &desktop->cycle;
    wm_window *window = cycle->items[cycle->selected];
    cycle->dirty = true;
    if (desktop->config.cycle.raise) {
        stack_raise(desktop, window);
    }
    if (desktop->config.cycle.draw_frame) {
        wm_rect frame = frame_rect(desktop, window);
        present_show_outline(desktop, &frame);
    }
}

static void close_session(wm_desktop *desktop)
{
    desktop->cycle.active = false;
    desktop->cycle.count = 0;
    if (desktop->config.cycle.draw_frame) {
        present_hide_outline(desktop);
    }
}

static void focus_and_raise(wm_desktop *desktop, wm_window *window)
{
    stack_raise(desktop, window);
    visibility_unminimize(desktop, transient_root(window));
    states_unshade(desktop, window);
    focus_set(desktop, window);
}

static void build_view(wm_desktop *desktop)
{
    cycle_session *cycle = &desktop->cycle;
    wm_cycle_view *view = &cycle->view;
    view->mode = cycle->grid.mode;
    view->preview = cycle->grid.preview;
    view->columns = cycle->grid.columns;
    view->rows = cycle->grid.rows;
    view->item_size = cycle->grid.item_size;
    view->monitor = cycle->grid.monitor;
    view->count = cycle->count;
    view->selected = cycle->selected;
    for (size_t i = 0; i < cycle->count; i++) {
        const wm_window *window = cycle->items[i];
        view->items[i] = (wm_cycle_item) {window->handle, window->title,
                                          window_is_minimized(window)};
    }
}

static void remove_item(cycle_session *cycle, size_t index)
{
    for (size_t i = index; i + 1 < cycle->count; i++) {
        cycle->items[i] = cycle->items[i + 1];
    }
    cycle->count--;
    if (cycle->selected > index) {
        cycle->selected--;
    } else if (cycle->selected >= cycle->count) {
        cycle->selected = 0;
    }
}

static wm_window *previous_matching(wm_desktop *desktop, unsigned mask)
{
    wm_window *focused = desktop->focus.focused;
    const window_list *mru = &desktop->mru;
    if (focused == NULL) {
        return NULL;
    }
    cycle_candidate candidate = {focused, mask | cycle_list_range(desktop)};
    size_t start = window_list_index_of(mru, focused);
    for (size_t i = 1; i < mru->count; i++) {
        wm_window *window = mru->items[(start + mru->count - i) % mru->count];
        if (window != focused && cycle_list_selects(desktop, window, &candidate)) {
            return window;
        }
    }
    return NULL;
}

static void cycle_activate(wm_desktop *desktop, wm_window *window)
{
    if (window->was_shown) {
        visibility_clear_show_desktop(desktop);
    }
    if (!visibility_on_current_workspace(desktop, window)) {
        workspaces_switch_without_focus(desktop, window->workspace, window);
    }
    focus_and_raise(desktop, window);
}
