#include "wrap.h"

#include <stdlib.h>

#include "desktop_state.h"
#include "monitors.h"
#include "workspaces.h"

enum {
    EDGE_RESET_MS = 250,
    MAX_SNAP_DRIFT = 15,
    WARP_NUMERATOR = 9,
    WARP_DENOMINATOR = 10,
    RESISTANCE_FACTOR = 5,
};

typedef struct wrap_edges {
    int min_x;
    int max_x;
    int min_y;
    int max_y;
} wrap_edges;

typedef struct wrap_axis {
    int position;
    int min;
    int max;
    int drift;
    int row_step;
    int column_step;
} wrap_axis;

static bool is_enabled(const wm_desktop *desktop, const wm_window *carried);
static wrap_edges screen_edges(const wm_desktop *desktop);
static void remember_origin(wm_desktop *desktop);
static void count_push(wm_desktop *desktop, int *counter, uint32_t time_ms);
static int push_back(int position, int min, int max);
static int wrap_along(wm_desktop *desktop, wm_window *carried, wrap_axis axis, int fallback);
static void warp(wm_desktop *desktop, int dx, int dy);

void wrap_at_screen_edges(wm_desktop *desktop, wm_window *carried, uint32_t time_ms)
{
    if (!is_enabled(desktop, carried)) {
        return;
    }
    wrap_state *wrap = &desktop->wrap;
    wrap_edges edges = screen_edges(desktop);
    remember_origin(desktop);
    int dx = push_back(desktop->pointer.x, edges.min_x, edges.max_x);
    int dy = push_back(desktop->pointer.y, edges.min_y, edges.max_y);
    if (dx != 0) {
        count_push(desktop, &wrap->edge_x, time_ms);
    }
    if (dy != 0) {
        count_push(desktop, &wrap->edge_y, time_ms);
    }
    wm_point pointer = desktop->pointer;
    if (wrap->edge_x > desktop->config.move.wrap_resistance) {
        wrap->edge_x = 0;
        wrap_axis axis = {pointer.x, edges.min_x, edges.max_x, pointer.y - wrap->original_y, 0, 1};
        dx = wrap_along(desktop, carried, axis, dx);
    }
    if (wrap->edge_y > desktop->config.move.wrap_resistance) {
        wrap->edge_y = 0;
        wrap_axis axis = {pointer.y, edges.min_y, edges.max_y, pointer.x - wrap->original_x, 1, 0};
        dy = wrap_along(desktop, carried, axis, dy);
    }
    if (dx != 0 || dy != 0) {
        warp(desktop, dx, dy);
    }
}

static bool is_enabled(const wm_desktop *desktop, const wm_window *carried)
{
    if (carried != NULL && !desktop->config.move.wrap_windows) {
        return false;
    }
    return desktop->config.move.wrap_resistance > 0 && desktop->workspaces.count > 1;
}

static wrap_edges screen_edges(const wm_desktop *desktop)
{
    wm_rect bounds = monitors_screen_bounds(desktop);
    return (wrap_edges) {bounds.x, wm_rect_right(&bounds) - 1, bounds.y,
                         wm_rect_bottom(&bounds) - 1};
}

static void remember_origin(wm_desktop *desktop)
{
    wrap_state *wrap = &desktop->wrap;
    if (wrap->edge_x == 0) {
        wrap->original_y = desktop->pointer.y;
    }
    if (wrap->edge_y == 0) {
        wrap->original_x = desktop->pointer.x;
    }
}

static void count_push(wm_desktop *desktop, int *counter, uint32_t time_ms)
{
    wrap_state *wrap = &desktop->wrap;
    if (time_ms - wrap->last_resist_ms > EDGE_RESET_MS) {
        *counter = 0;
    } else {
        (*counter)++;
    }
    wrap->last_resist_ms = time_ms;
}

static int push_back(int position, int min, int max)
{
    if (position == min) {
        return 1;
    }
    return position == max ? -1 : 0;
}

static int wrap_along(wm_desktop *desktop, wm_window *carried, wrap_axis axis, int fallback)
{
    int span = axis.max - axis.min;
    int delta = WARP_NUMERATOR * span / WARP_DENOMINATOR;
    int resisted = span - RESISTANCE_FACTOR * desktop->config.move.wrap_resistance;
    delta = wm_max(delta, resisted);
    desktop->wrap.last_resist_ms = 0;
    if (abs(axis.drift) >= MAX_SNAP_DRIFT) {
        return fallback;
    }
    int step = axis.position == axis.min ? -1 : 1;
    bool moved = workspaces_move_changed(desktop, step * axis.row_step, step * axis.column_step, carried);
    return moved ? -step * delta : fallback;
}

static void warp(wm_desktop *desktop, int dx, int dy)
{
    wm_point point = {desktop->pointer.x + dx, desktop->pointer.y + dy};
    desktop->ports.commands.warp_pointer(desktop->ports.commands.self, point);
    desktop->pointer = monitors_pointer(desktop);
}
