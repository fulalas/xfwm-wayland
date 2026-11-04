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

typedef struct snap_segment {
    int low;
    int high;
} snap_segment;

typedef struct snap_axes {
    snap_axis *x;
    snap_axis *y;
    wm_rect frame;
} snap_axes;

static snap_axis make_axis(int start, int size, int moved, int snap_width);
static void snap_to_obstacles(const wm_desktop *desktop, const wm_window *window,
                              const snap_axes *axes);
static void snap_to_display(const wm_desktop *desktop, snap_axis *axis, snap_segment display);
static void snap_to_obstacle(const wm_desktop *desktop, snap_axis *axis, snap_segment obstacle);
static bool resist_allows(const wm_desktop *desktop, bool condition);
static bool spans_overlap(int low_a, int high_a, int low_b, int high_b);
static int closest_edge(const wm_desktop *desktop, snap_segment own, int edge,
                        const snap_obstacles *obstacles);

void snap_position(const wm_desktop *desktop, wm_window *window, wm_point previous)
{
    int width = desktop->config.move.snap_width;
    wm_rect frame = frame_rect(desktop, window);
    wm_extents extents = frame_extents(desktop, window);
    snap_axis x = make_axis(frame.x, frame.width, window->client.x - previous.x, width);
    snap_axis y = make_axis(frame.y, frame.height, window->client.y - previous.y, width);
    if (desktop->config.move.snap_to_border) {
        wm_rect monitor = monitors_full_at(desktop, wm_rect_center(&frame));
        snap_to_display(desktop, &x, (snap_segment) {monitor.x, wm_rect_right(&monitor)});
        snap_to_display(desktop, &y, (snap_segment) {monitor.y, wm_rect_bottom(&monitor)});
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
    snap_segment own = {frame.y - 1, wm_rect_bottom(&frame) + 1};
    for (size_t i = 0; i < obstacles.count; i++) {
        wm_rect *rect = &obstacles.rects[i];
        *rect = (wm_rect) {rect->y, rect->x, rect->height, rect->width};
    }
    int closest = closest_edge(desktop, own, edge, &obstacles);
    snap_obstacles_free(&obstacles);
    return closest;
}

int snap_closest_edge_y(const wm_desktop *desktop, const wm_window *window, int edge)
{
    wm_rect frame = frame_rect(desktop, window);
    snap_obstacles obstacles = snap_obstacles_collect(desktop, window);
    snap_segment own = {frame.x - 1, wm_rect_right(&frame) + 1};
    int closest = closest_edge(desktop, own, edge, &obstacles);
    snap_obstacles_free(&obstacles);
    return closest;
}

static snap_axis make_axis(int start, int size, int moved, int snap_width)
{
    snap_axis axis = {start, size, moved < 0, moved > 0, snap_width + 1, start};
    return axis;
}

static void snap_to_obstacles(const wm_desktop *desktop, const wm_window *window,
                              const snap_axes *axes)
{
    const wm_rect *frame = &axes->frame;
    snap_obstacles obstacles = snap_obstacles_collect(desktop, window);
    for (size_t i = 0; i < obstacles.count; i++) {
        const wm_rect *other = &obstacles.rects[i];
        if (spans_overlap(other->y, wm_rect_bottom(other), frame->y, wm_rect_bottom(frame))) {
            snap_to_obstacle(desktop, axes->x, (snap_segment) {other->x, wm_rect_right(other)});
        }
        if (spans_overlap(other->x, wm_rect_right(other), frame->x, wm_rect_right(frame))) {
            snap_to_obstacle(desktop, axes->y, (snap_segment) {other->y, wm_rect_bottom(other)});
        }
    }
    snap_obstacles_free(&obstacles);
}

static void snap_to_display(const wm_desktop *desktop, snap_axis *axis, snap_segment display)
{
    int end = axis->start + axis->size;
    if (abs(display.low - axis->start) < abs(display.high - end)) {
        if (resist_allows(desktop, axis->start <= display.low && axis->decreasing)) {
            axis->best_delta = abs(display.low - axis->start);
            axis->best_start = display.low;
        }
        return;
    }
    if (resist_allows(desktop, end >= display.high && axis->increasing)) {
        axis->best_delta = abs(display.high - end);
        axis->best_start = display.high - axis->size;
    }
}

static void snap_to_obstacle(const wm_desktop *desktop, snap_axis *axis, snap_segment obstacle)
{
    int end = axis->start + axis->size;
    int delta = abs(obstacle.high - axis->start);
    if (delta < axis->best_delta
        && resist_allows(desktop, axis->start <= obstacle.high && axis->decreasing)) {
        axis->best_delta = delta;
        axis->best_start = obstacle.high;
    }
    delta = abs(obstacle.low - end);
    if (delta < axis->best_delta
        && resist_allows(desktop, end >= obstacle.low && axis->increasing)) {
        axis->best_delta = delta;
        axis->best_start = obstacle.low - axis->size;
    }
}

static bool resist_allows(const wm_desktop *desktop, bool condition)
{
    return !desktop->config.move.snap_resist || condition;
}

static bool spans_overlap(int low_a, int high_a, int low_b, int high_b)
{
    return low_a <= high_b && high_a >= low_b;
}

static int closest_edge(const wm_desktop *desktop, snap_segment own, int edge,
                        const snap_obstacles *obstacles)
{
    int width = desktop->config.move.snap_width;
    int closest = edge + width + 2;
    for (size_t i = 0; i < obstacles->count; i++) {
        const wm_rect *rect = &obstacles->rects[i];
        if (!spans_overlap(own.low, own.high, rect->x - 1, wm_rect_right(rect) + 1)) {
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
