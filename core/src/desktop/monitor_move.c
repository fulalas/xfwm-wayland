#include "monitor_move.h"

#include <limits.h>
#include <stdlib.h>

#include "desktop_state.h"
#include "maximize.h"
#include "monitors.h"
#include "states.h"
#include "tile.h"

enum {
    NO_MONITOR = -1,
};

static const double ROUNDING = 0.5;

static size_t current_monitor(const wm_desktop *desktop, const wm_window *window);
static int find_target(const wm_desktop *desktop, const wm_window *window,
                       monitor_direction direction);
static bool is_aligned(const wm_rect *current, const wm_rect *other, monitor_direction direction);
static int midpoint_offset(const wm_rect *current, const wm_rect *other,
                           monitor_direction direction);
static wm_point transform(wm_point point, const wm_rect *from, const wm_rect *to);
static void move_remembered_geometry(wm_window *window, const wm_rect *from, const wm_rect *to);
static void refit(wm_desktop *desktop, wm_window *window);

bool monitor_move_possible(const wm_desktop *desktop, const wm_window *window,
                           monitor_direction direction)
{
    return find_target(desktop, window, direction) != NO_MONITOR;
}

void monitor_move(wm_desktop *desktop, wm_window *window, monitor_direction direction)
{
    int target = find_target(desktop, window, direction);
    if (target != NO_MONITOR) {
        monitor_move_to(desktop, window, (size_t) target);
    }
}

void monitor_move_to(wm_desktop *desktop, wm_window *window, size_t target)
{
    size_t current = current_monitor(desktop, window);
    if (target >= monitors_count(desktop) || target == current) {
        return;
    }
    wm_rect from = monitors_work_area_of(desktop, current);
    wm_rect to = monitors_work_area_of(desktop, target);
    wm_point position = transform((wm_point) {window->client.x, window->client.y}, &from, &to);
    window->client.x = position.x;
    window->client.y = position.y;
    move_remembered_geometry(window, &from, &to);
    refit(desktop, window);
    window_mark_dirty(window);
}

static size_t current_monitor(const wm_desktop *desktop, const wm_window *window)
{
    wm_point middle = {window->client.x + window->client.width / 2,
                       window->client.y + window->client.height / 2};
    return monitors_index_at(desktop, middle);
}

static int find_target(const wm_desktop *desktop, const wm_window *window,
                       monitor_direction direction)
{
    size_t count = monitors_count(desktop);
    if (count == 0) {
        return NO_MONITOR;
    }
    size_t current = current_monitor(desktop, window);
    wm_rect current_rect = monitors_full_rect(desktop, current);
    int best = NO_MONITOR;
    int best_offset = INT_MAX;
    for (size_t i = 0; i < count; i++) {
        wm_rect other = monitors_full_rect(desktop, i);
        if (i == current || !is_aligned(&current_rect, &other, direction)) {
            continue;
        }
        int offset = midpoint_offset(&current_rect, &other, direction);
        if (offset >= 0 && offset < best_offset) {
            best_offset = offset;
            best = (int) i;
        }
    }
    return best;
}

static bool is_aligned(const wm_rect *current, const wm_rect *other, monitor_direction direction)
{
    switch (direction) {
    case MONITOR_LEFT:
        return current->x == wm_rect_right(other);
    case MONITOR_RIGHT:
        return other->x == wm_rect_right(current);
    case MONITOR_DOWN:
        return other->y == wm_rect_bottom(current);
    case MONITOR_UP:
        return current->y == wm_rect_bottom(other);
    }
    return false;
}

static int midpoint_offset(const wm_rect *current, const wm_rect *other,
                           monitor_direction direction)
{
    bool horizontal = direction == MONITOR_LEFT || direction == MONITOR_RIGHT;
    int low = horizontal ? wm_max(current->y, other->y) : wm_max(current->x, other->x);
    int high_a = horizontal ? wm_rect_bottom(current) : wm_rect_right(current);
    int high_b = horizontal ? wm_rect_bottom(other) : wm_rect_right(other);
    int high = wm_min(high_a, high_b);
    if (low >= high) {
        return -1;
    }
    int middle = horizontal ? current->y + (current->height >> 1)
                            : current->x + (current->width >> 1);
    return abs(low + ((high - low) >> 1) - middle);
}

static wm_point transform(wm_point point, const wm_rect *from, const wm_rect *to)
{
    double ratio_x = from->width ? (point.x - from->x) / (double) from->width : 0.0;
    double ratio_y = from->height ? (point.y - from->y) / (double) from->height : 0.0;
    return (wm_point) {(int) (to->x + ratio_x * to->width + ROUNDING),
                       (int) (to->y + ratio_y * to->height + ROUNDING)};
}

static void move_remembered_geometry(wm_window *window, const wm_rect *from, const wm_rect *to)
{
    if (window->restore_size_pos) {
        wm_point saved = transform((wm_point) {window->saved.x, window->saved.y}, from, to);
        window->saved.x = saved.x;
        window->saved.y = saved.y;
    }
    window->fullscreen_area = (wm_rect) {0, 0, 0, 0};
    if (window_is_fullscreen(window)) {
        wm_point origin = {window->pre_fullscreen.x, window->pre_fullscreen.y};
        wm_point moved = transform(origin, from, to);
        window->pre_fullscreen.x = moved.x;
        window->pre_fullscreen.y = moved.y;
    }
}

static void refit(wm_desktop *desktop, wm_window *window)
{
    if (window_is_fullscreen(window)) {
        states_update_fullscreen_size(desktop, window);
    } else if (window_has_state(window, WM_STATE_MAXIMIZED)) {
        maximize_update_size(desktop, window);
    }
    tile_update_size(desktop, window);
}
