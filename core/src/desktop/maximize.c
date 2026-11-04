#include "maximize.h"

#include "desktop_state.h"
#include "frame.h"
#include "monitors.h"
#include "size_hints.h"
#include "states.h"

typedef struct maximize_request {
    wm_point point;
    uint32_t mode;
    bool restore_position;
} maximize_request;

static bool toggle(wm_desktop *desktop, wm_window *window, const maximize_request *request);
static void new_state(wm_window *window, wm_rect *rect, uint32_t mode);
static void toggle_axis(wm_window *window, wm_rect *rect, uint32_t axis);
static bool new_size(const wm_desktop *desktop, const wm_window *window, wm_rect *rect,
                     wm_point point);

bool maximize_toggle(wm_desktop *desktop, wm_window *window, uint32_t mode)
{
    maximize_request request = {frame_center(desktop, window), mode, true};
    return toggle(desktop, window, &request);
}

bool maximize_toggle_in_place(wm_desktop *desktop, wm_window *window, uint32_t mode)
{
    maximize_request request = {frame_center(desktop, window), mode, false};
    return toggle(desktop, window, &request);
}

bool maximize_toggle_at(wm_desktop *desktop, wm_window *window, wm_point point, uint32_t mode)
{
    maximize_request request = {point, mode, false};
    return toggle(desktop, window, &request);
}

void maximize_save_geometry(wm_window *window)
{
    if (!window->restore_size_pos) {
        window->saved = window->client;
    }
}

bool maximize_restore_geometry(wm_window *window)
{
    if (!window->restore_size_pos) {
        return false;
    }
    window->client = window->saved;
    window->restore_size_pos = false;
    window_mark_dirty(window);
    return true;
}

void maximize_update_size(wm_desktop *desktop, wm_window *window)
{
    wm_rect rect = window->client;
    if (window_has_state(window, WM_STATE_MAXIMIZED)
        && new_size(desktop, window, &rect, frame_center(desktop, window))) {
        window->client = rect;
        window_mark_dirty(window);
    }
}

void maximize_remove_flag(wm_window *window)
{
    window->states &= ~(uint32_t) WM_STATE_MAXIMIZED;
    window->restore_size_pos = false;
    window_mark_dirty(window);
}

static bool toggle(wm_desktop *desktop, wm_window *window, const maximize_request *request)
{
    if (!window_can_maximize(window)) {
        return false;
    }
    wm_rect rect = window->client;
    if (request->restore_position && (request->mode & WM_STATE_MAXIMIZED)) {
        maximize_save_geometry(window);
    }
    uint32_t old_states = window->states;
    bool old_restore = window->restore_size_pos;
    new_state(window, &rect, request->mode);
    if (!new_size(desktop, window, &rect, request->point)) {
        window->states = old_states;
        window->restore_size_pos = old_restore;
        return false;
    }
    window->tile = TILE_NONE;
    window->client = rect;
    window_mark_dirty(window);
    if (request->restore_position && window_is_shaded(window)) {
        states_unshade(desktop, window);
    }
    return true;
}

static void new_state(wm_window *window, wm_rect *rect, uint32_t mode)
{
    if ((mode & WM_STATE_MAXIMIZED) == WM_STATE_MAXIMIZED
        && window_has_state(window, WM_STATE_MAXIMIZED)) {
        window->states &= ~(uint32_t) WM_STATE_MAXIMIZED;
        window->restore_size_pos = false;
        *rect = window->saved;
        return;
    }
    toggle_axis(window, rect, mode & WM_STATE_MAXIMIZED_HORZ);
    toggle_axis(window, rect, mode & WM_STATE_MAXIMIZED_VERT);
}

static void toggle_axis(wm_window *window, wm_rect *rect, uint32_t axis)
{
    if (axis == 0) {
        return;
    }
    if (!window_has_state(window, axis)) {
        window->states |= axis;
        window->restore_size_pos = true;
        return;
    }
    window->states &= ~axis;
    if (!window_has_state(window, WM_STATE_MAXIMIZED)) {
        window->restore_size_pos = false;
    }
    *rect = window->saved;
}

static bool new_size(const wm_desktop *desktop, const wm_window *window, wm_rect *rect,
                     wm_point point)
{
    wm_rect full = monitors_work_area_at(desktop, point);
    wm_extents extents = frame_extents(desktop, window);
    if (window_has_state(window, WM_STATE_MAXIMIZED_HORZ)) {
        rect->x = full.x + extents.left;
        rect->width = full.width - extents.left - extents.right;
    }
    if (window_has_state(window, WM_STATE_MAXIMIZED_VERT)) {
        rect->y = full.y + extents.top;
        rect->height = full.height - extents.top - extents.bottom;
    }
    return size_hints_allow(window, (wm_size) {rect->width, rect->height});
}
