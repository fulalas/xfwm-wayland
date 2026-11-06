#include "present.h"

#include <stdlib.h>
#include <string.h>

#include "cycle.h"
#include "desktop_state.h"
#include "frame.h"
#include "highlight.h"
#include "launches.h"
#include "menu.h"
#include "size_hints.h"
#include "timers.h"
#include "view.h"
#include "workspaces.h"
#include "zoom.h"

static void flush_configures(wm_desktop *desktop);
static void flush_windows(wm_desktop *desktop);
static void flush_stacking(wm_desktop *desktop);
static void flush_focus(wm_desktop *desktop);
static void flush_workspaces(wm_desktop *desktop);
static void flush_outline(wm_desktop *desktop);
static void flush_size(wm_desktop *desktop);
static void flush_cursor(wm_desktop *desktop);
static void flush_busy(wm_desktop *desktop);

void present_flush(wm_desktop *desktop)
{
    flush_configures(desktop);
    flush_windows(desktop);
    flush_stacking(desktop);
    flush_focus(desktop);
    flush_workspaces(desktop);
    cycle_present(desktop);
    menu_present(desktop);
    highlight_present(desktop);
    flush_outline(desktop);
    flush_size(desktop);
    zoom_present(desktop);
    flush_cursor(desktop);
    flush_busy(desktop);
    timers_reschedule(desktop);
}

void present_show_outline(wm_desktop *desktop, const wm_rect *rect)
{
    present_state *present = &desktop->present;
    if (present->outline_visible && wm_rect_equals(&present->outline, rect)) {
        return;
    }
    present->outline_visible = true;
    present->outline = *rect;
    present->outline_dirty = true;
}

void present_hide_outline(wm_desktop *desktop)
{
    present_state *present = &desktop->present;
    if (present->outline_visible) {
        present->outline_visible = false;
        present->outline_dirty = true;
    }
}

void present_set_cursor(wm_desktop *desktop, wm_cursor_shape shape)
{
    desktop->present.cursor = shape;
}

void present_request_configure(wm_window *window)
{
    window->configure_pending = true;
}

void present_mark_workspaces(wm_desktop *desktop)
{
    desktop->present.workspaces_dirty = true;
}

static void flush_configures(wm_desktop *desktop)
{
    const wm_commands *commands = &desktop->ports.commands;
    for (size_t i = 0; i < desktop->stack.count; i++) {
        wm_window *window = desktop->stack.items[i];
        bool unchanged = wm_rect_equals(&window->configured, &window->client);
        if (window->frozen || (!window->configure_pending && unchanged)) {
            continue;
        }
        window->configure_pending = false;
        window->configured = window->client;
        window->dirty = true;
        commands->configure(commands->self, window->handle, &window->client);
    }
}

static void flush_windows(wm_desktop *desktop)
{
    const wm_presenter *presenter = &desktop->ports.presenter;
    for (size_t i = 0; i < desktop->stack.count; i++) {
        wm_window *window = desktop->stack.items[i];
        if (!window->dirty) {
            continue;
        }
        window->dirty = false;
        view_refresh(desktop, window);
        presenter->window_changed(presenter->self, window->handle, &window->view);
    }
}

static void flush_stacking(wm_desktop *desktop)
{
    if (!desktop->present.stacking_dirty) {
        return;
    }
    size_t count = desktop->stack.count;
    void **handles = malloc((count ? count : 1) * sizeof(handles[0]));
    if (handles == NULL) {
        return;
    }
    desktop->present.stacking_dirty = false;
    for (size_t i = 0; i < count; i++) {
        handles[i] = desktop->stack.items[i]->handle;
    }
    const wm_presenter *presenter = &desktop->ports.presenter;
    presenter->stacking_changed(presenter->self, (void *const *) handles, count);
    free(handles);
}

static void flush_focus(wm_desktop *desktop)
{
    present_state *present = &desktop->present;
    wm_window *focused = desktop->focus.focused;
    void *handle = focused ? focused->handle : NULL;
    if (present->focus_reported && present->reported_focus == handle) {
        return;
    }
    present->focus_reported = true;
    present->reported_focus = handle;
    desktop->ports.presenter.focus_changed(desktop->ports.presenter.self, handle);
}

static void flush_workspaces(wm_desktop *desktop)
{
    if (!desktop->present.workspaces_dirty) {
        return;
    }
    desktop->present.workspaces_dirty = false;
    wm_workspaces_view view = workspaces_view(desktop);
    desktop->ports.presenter.workspaces_changed(desktop->ports.presenter.self, &view);
}

static void flush_outline(wm_desktop *desktop)
{
    present_state *present = &desktop->present;
    const wm_presenter *presenter = &desktop->ports.presenter;
    if (!present->outline_dirty) {
        return;
    }
    present->outline_dirty = false;
    if (present->outline_visible) {
        presenter->outline_shown(presenter->self, &present->outline);
    } else {
        presenter->outline_hidden(presenter->self);
    }
}

static void flush_size(wm_desktop *desktop)
{
    present_state *present = &desktop->present;
    const wm_presenter *presenter = &desktop->ports.presenter;
    const move_session *session = &desktop->session;
    wm_size_view view = {0};
    bool visible = session->kind == SESSION_RESIZE && size_hints_has_steps(session->window);
    if (visible) {
        wm_size steps = size_hints_steps_of(session->window);
        view = (wm_size_view) {frame_rect(desktop, session->window), steps.width, steps.height};
    }
    if (visible == present->size_visible &&
        (!visible || memcmp(&view, &present->size, sizeof(view)) == 0)) {
        return;
    }
    present->size_visible = visible;
    present->size = view;
    if (visible) {
        presenter->size_shown(presenter->self, &present->size);
    } else {
        presenter->size_hidden(presenter->self);
    }
}

static void flush_cursor(wm_desktop *desktop)
{
    present_state *present = &desktop->present;
    wm_cursor_shape shape = present->cursor;
    if (shape == WM_CURSOR_DEFAULT && launches_busy(desktop)) {
        shape = WM_CURSOR_BUSY;
    }
    if (present->cursor_reported && present->reported_cursor == shape) {
        return;
    }
    present->cursor_reported = true;
    present->reported_cursor = shape;
    desktop->ports.presenter.cursor_changed(desktop->ports.presenter.self, shape);
}

static void flush_busy(wm_desktop *desktop)
{
    bool busy = launches_busy(desktop);
    if (busy != desktop->present.busy) {
        desktop->present.busy = busy;
        desktop->ports.presenter.busy_changed(desktop->ports.presenter.self, busy);
    }
}
