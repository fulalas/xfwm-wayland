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
    int *edge_count;
    int position;
    int min;
    int max;
    int drift;
    int push;
    move_direction toward_min;
    move_direction toward_max;
} wrap_axis;

static bool is_enabled(const wm_desktop *desktop, const wm_window *carried);
static wrap_edges screen_edges(const wm_desktop *desktop);
static void remember_origin(wm_desktop *desktop);
static wm_point count_pushes(wm_desktop *desktop, const wrap_edges *edges, uint32_t time_ms);
static int wrap_if_resisted(wm_desktop *desktop, wm_window *carried, const wrap_axis *axis);
static void count_push(wm_desktop *desktop, int *counter, uint32_t time_ms);
static int push_back(int position, int min, int max);
static int wrap_along(wm_desktop *desktop, wm_window *carried, const wrap_axis *axis);
static void warp(wm_desktop *desktop, int dx, int dy);

void wrap_at_screen_edges(wm_desktop *desktop, wm_window *carried, uint32_t time_ms)
{
    if (!is_enabled(desktop, carried)) {
        return;
    }
    wrap_state *wrap = &desktop->wrap;
    wrap_edges edges = screen_edges(desktop);
    remember_origin(desktop);
    wm_point push = count_pushes(desktop, &edges, time_ms);
    wm_point pointer = desktop->pointer;
    wrap_axis horizontal = {&wrap->edge_x, pointer.x, edges.min_x, edges.max_x,
                            pointer.y - wrap->original_y, push.x, DIRECTION_LEFT, DIRECTION_RIGHT};
    wrap_axis vertical = {&wrap->edge_y, pointer.y, edges.min_y, edges.max_y,
                          pointer.x - wrap->original_x, push.y, DIRECTION_UP, DIRECTION_DOWN};
    push.x = wrap_if_resisted(desktop, carried, &horizontal);
    push.y = wrap_if_resisted(desktop, carried, &vertical);
    if (push.x != 0 || push.y != 0) {
        warp(desktop, push.x, push.y);
    }
}

static wm_point count_pushes(wm_desktop *desktop, const wrap_edges *edges, uint32_t time_ms)
{
    wrap_state *wrap = &desktop->wrap;
    wm_point push = {push_back(desktop->pointer.x, edges->min_x, edges->max_x),
                     push_back(desktop->pointer.y, edges->min_y, edges->max_y)};
    if (push.x != 0) {
        count_push(desktop, &wrap->edge_x, time_ms);
    }
    if (push.y != 0) {
        count_push(desktop, &wrap->edge_y, time_ms);
    }
    return push;
}

static int wrap_if_resisted(wm_desktop *desktop, wm_window *carried, const wrap_axis *axis)
{
    if (*axis->edge_count <= desktop->config.move.wrap_resistance) {
        return axis->push;
    }
    *axis->edge_count = 0;
    return wrap_along(desktop, carried, axis);
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

static int wrap_along(wm_desktop *desktop, wm_window *carried, const wrap_axis *axis)
{
    int span = axis->max - axis->min;
    int delta = WARP_NUMERATOR * span / WARP_DENOMINATOR;
    int resisted = span - RESISTANCE_FACTOR * desktop->config.move.wrap_resistance;
    delta = wm_max(delta, resisted);
    desktop->wrap.last_resist_ms = 0;
    if (abs(axis->drift) >= MAX_SNAP_DRIFT) {
        return axis->push;
    }
    bool at_min = axis->position == axis->min;
    move_direction direction = at_min ? axis->toward_min : axis->toward_max;
    bool moved = workspaces_move_changed(desktop, direction, carried);
    int away = at_min ? delta : -delta;
    return moved ? away : axis->push;
}

static void warp(wm_desktop *desktop, int dx, int dy)
{
    wm_point point = {desktop->pointer.x + dx, desktop->pointer.y + dy};
    desktop->ports.commands.warp_pointer(desktop->ports.commands.self, point);
    desktop->pointer = monitors_pointer(desktop);
}
