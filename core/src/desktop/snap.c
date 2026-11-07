#include "snap.h"

#include <stdlib.h>

#include "desktop_state.h"
#include "frame.h"
#include "monitors.h"
#include "snap_obstacles.h"

typedef struct snap_axis {
    int start;
    int size;
    bool decreasing;
    bool increasing;
    int best_delta;
    int best_start;
} snap_axis;

typedef struct snap_axes {
    snap_axis *x;
    snap_axis *y;
    wm_rect frame;
} snap_axes;

typedef struct edge_query {
    wm_segment own;
    int edge;
    int snap_width;
} edge_query;

static snap_axis make_axis(wm_segment span, int moved, int snap_width);
static void snap_to_obstacles(const wm_desktop *desktop, const wm_window *window,
                              const snap_axes *axes);
static void snap_to_display(const wm_desktop *desktop, snap_axis *axis, wm_segment display);
static void snap_to_obstacle(const wm_desktop *desktop, snap_axis *axis, wm_segment obstacle);
static bool resist_allows(const wm_desktop *desktop, bool condition);
static int closest_edge(const edge_query *query, const snap_obstacles *obstacles);

void snap_position(const wm_desktop *desktop, wm_window *window, wm_point previous)
{
    int width = desktop->config.move.snap_width;
    wm_rect frame = frame_rect(desktop, window);
    wm_extents extents = frame_extents(desktop, window);
    snap_axis x = make_axis(wm_rect_columns(&frame), window->client.x - previous.x, width);
    snap_axis y = make_axis(wm_rect_rows(&frame), window->client.y - previous.y, width);
    if (desktop->config.move.snap_to_border) {
        wm_rect monitor = monitors_full_at(desktop, wm_rect_center(&frame));
        snap_to_display(desktop, &x, wm_rect_columns(&monitor));
        snap_to_display(desktop, &y, wm_rect_rows(&monitor));
    }
    snap_axes axes = {&x, &y, frame};
    snap_to_obstacles(desktop, window, &axes);
    if (x.best_delta <= width) {
        window->client.x = x.best_start + extents.left;
    }
    if (y.best_delta <= width) {
        window->client.y = y.best_start + extents.top;
    }
}

int snap_closest_edge_x(const wm_desktop *desktop, const wm_window *window, int edge)
{
    wm_rect frame = frame_rect(desktop, window);
    snap_obstacles obstacles = snap_obstacles_collect(desktop, window);
    edge_query query = {{frame.y - 1, wm_rect_bottom(&frame) + 1}, edge,
                        desktop->config.move.snap_width};
    for (size_t i = 0; i < obstacles.count; i++) {
        wm_rect *rect = &obstacles.rects[i];
        *rect = (wm_rect) {rect->y, rect->x, rect->height, rect->width};
    }
    int closest = closest_edge(&query, &obstacles);
    snap_obstacles_free(&obstacles);
    return closest;
}

int snap_closest_edge_y(const wm_desktop *desktop, const wm_window *window, int edge)
{
    wm_rect frame = frame_rect(desktop, window);
    snap_obstacles obstacles = snap_obstacles_collect(desktop, window);
    edge_query query = {{frame.x - 1, wm_rect_right(&frame) + 1}, edge,
                        desktop->config.move.snap_width};
    int closest = closest_edge(&query, &obstacles);
    snap_obstacles_free(&obstacles);
    return closest;
}

static snap_axis make_axis(wm_segment span, int moved, int snap_width)
{
    snap_axis axis = {span.start, span.end - span.start, moved < 0, moved > 0, snap_width + 1,
                      span.start};
    return axis;
}

static void snap_to_obstacles(const wm_desktop *desktop, const wm_window *window,
                              const snap_axes *axes)
{
    const wm_rect *frame = &axes->frame;
    snap_obstacles obstacles = snap_obstacles_collect(desktop, window);
    for (size_t i = 0; i < obstacles.count; i++) {
        const wm_rect *other = &obstacles.rects[i];
        if (wm_segments_touch(wm_rect_rows(other), wm_rect_rows(frame))) {
            snap_to_obstacle(desktop, axes->x, wm_rect_columns(other));
        }
        if (wm_segments_touch(wm_rect_columns(other), wm_rect_columns(frame))) {
            snap_to_obstacle(desktop, axes->y, wm_rect_rows(other));
        }
    }
    snap_obstacles_free(&obstacles);
}

static void snap_to_display(const wm_desktop *desktop, snap_axis *axis, wm_segment display)
{
    int end = axis->start + axis->size;
    if (abs(display.start - axis->start) < abs(display.end - end)) {
        if (resist_allows(desktop, axis->start <= display.start && axis->decreasing)) {
            axis->best_delta = abs(display.start - axis->start);
            axis->best_start = display.start;
        }
        return;
    }
    if (resist_allows(desktop, end >= display.end && axis->increasing)) {
        axis->best_delta = abs(display.end - end);
        axis->best_start = display.end - axis->size;
    }
}

static void snap_to_obstacle(const wm_desktop *desktop, snap_axis *axis, wm_segment obstacle)
{
    int end = axis->start + axis->size;
    int delta = abs(obstacle.end - axis->start);
    if (delta < axis->best_delta
        && resist_allows(desktop, axis->start <= obstacle.end && axis->decreasing)) {
        axis->best_delta = delta;
        axis->best_start = obstacle.end;
    }
    delta = abs(obstacle.start - end);
    if (delta < axis->best_delta
        && resist_allows(desktop, end >= obstacle.start && axis->increasing)) {
        axis->best_delta = delta;
        axis->best_start = obstacle.start - axis->size;
    }
}

static bool resist_allows(const wm_desktop *desktop, bool condition)
{
    return !desktop->config.move.snap_resist || condition;
}

static int closest_edge(const edge_query *query, const snap_obstacles *obstacles)
{
    int edge = query->edge;
    int width = query->snap_width;
    int closest = edge + width + 2;
    for (size_t i = 0; i < obstacles->count; i++) {
        const wm_rect *rect = &obstacles->rects[i];
        wm_segment widened = {rect->x - 1, wm_rect_right(rect) + 1};
        if (!wm_segments_touch(query->own, widened)) {
            continue;
        }
        if (abs(rect->y - edge) < abs(closest - edge)) {
            closest = rect->y;
        }
        if (abs(wm_rect_bottom(rect) - edge) < abs(closest - edge)) {
            closest = wm_rect_bottom(rect);
        }
    }
    return abs(closest - edge) > width ? edge : closest;
}
