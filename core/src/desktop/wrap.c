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

static bool is_enabled(const wm_desktop *desktop, const wm_window *carried);
static wrap_edges screen_edges(const wm_desktop *desktop);
static void remember_origin(wm_desktop *desktop);
static void count_push(wm_desktop *desktop, int *counter, uint32_t time_ms);
static int push_back(int position, int min, int max);
static int wrap_horizontally(wm_desktop *desktop, wm_window *carried, const wrap_edges *edges,
                             int fallback);
static void reset_vertical(wm_desktop *desktop);
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
    if (wrap->edge_x > desktop->config.move.wrap_resistance) {
        dx = wrap_horizontally(desktop, carried, &edges, dx);
    }
    if (wrap->edge_y > desktop->config.move.wrap_resistance) {
        reset_vertical(desktop);
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

static int wrap_horizontally(wm_desktop *desktop, wm_window *carried, const wrap_edges *edges,
                             int fallback)
{
    wrap_state *wrap = &desktop->wrap;
    wm_point pointer = desktop->pointer;
    int span = edges->max_x - edges->min_x;
    int delta = WARP_NUMERATOR * span / WARP_DENOMINATOR;
    int resisted = span - RESISTANCE_FACTOR * desktop->config.move.wrap_resistance;
    delta = wm_max(delta, resisted);
    wrap->edge_x = 0;
    wrap->last_resist_ms = 0;
    if (abs(pointer.y - wrap->original_y) >= MAX_SNAP_DRIFT) {
        return fallback;
    }
    if (pointer.x == edges->min_x) {
        return workspaces_move_changed(desktop, -1, carried) ? delta : fallback;
    }
    return workspaces_move_changed(desktop, 1, carried) ? -delta : fallback;
}

static void reset_vertical(wm_desktop *desktop)
{
    desktop->wrap.edge_y = 0;
    desktop->wrap.last_resist_ms = 0;
}

static void warp(wm_desktop *desktop, int dx, int dy)
{
    desktop->pointer.x += dx;
    desktop->pointer.y += dy;
    desktop->ports.commands.warp_pointer(desktop->ports.commands.self, desktop->pointer);
}
