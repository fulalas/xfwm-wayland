#include "snap_obstacles.h"

#include <stdlib.h>

#include "desktop_state.h"
#include "frame.h"
#include "monitors.h"

enum {
    STRUT_COUNT = 4,
};

static bool is_window_obstacle(const wm_desktop *desktop, const wm_window *window,
                               const wm_window *other);
static void add_struts(const wm_desktop *desktop, const wm_window *window,
                       snap_obstacles *obstacles);
static void add_rect(snap_obstacles *obstacles, wm_rect rect);

snap_obstacles snap_obstacles_collect(const wm_desktop *desktop, const wm_window *window)
{
    snap_obstacles obstacles = {0};
    const wm_move_config *config = &desktop->config.move;
    if (!config->snap_to_windows && !config->snap_to_border) {
        return obstacles;
    }
    obstacles.rects = malloc((desktop->stack.count + STRUT_COUNT) * sizeof(wm_rect));
    if (obstacles.rects == NULL) {
        return obstacles;
    }
    for (size_t i = 0; i < desktop->stack.count; i++) {
        const wm_window *other = desktop->stack.items[i];
        if (is_window_obstacle(desktop, window, other)) {
            add_rect(&obstacles, frame_rect(desktop, other));
        }
    }
    if (config->snap_to_border) {
        add_struts(desktop, window, &obstacles);
    }
    return obstacles;
}

void snap_obstacles_free(snap_obstacles *obstacles)
{
    free(obstacles->rects);
    obstacles->rects = NULL;
    obstacles->count = 0;
}

static bool is_window_obstacle(const wm_desktop *desktop, const wm_window *window,
                               const wm_window *other)
{
    if (other == window || !other->visible) {
        return false;
    }
    const wm_move_config *config = &desktop->config.move;
    if (config->snap_to_windows && other->layer == window->layer) {
        return true;
    }
    return config->snap_to_border && other->type == WM_TYPE_DOCK;
}

static void add_struts(const wm_desktop *desktop, const wm_window *window,
                       snap_obstacles *obstacles)
{
    wm_point center = frame_center(desktop, window);
    wm_rect monitor = monitors_full_at(desktop, center);
    wm_rect work = monitors_work_area_at(desktop, center);
    add_rect(obstacles, (wm_rect) {monitor.x, monitor.y, work.x - monitor.x, monitor.height});
    add_rect(obstacles, (wm_rect) {wm_rect_right(&work), monitor.y,
                                   wm_rect_right(&monitor) - wm_rect_right(&work),
                                   monitor.height});
    add_rect(obstacles, (wm_rect) {monitor.x, monitor.y, monitor.width, work.y - monitor.y});
    add_rect(obstacles, (wm_rect) {monitor.x, wm_rect_bottom(&work), monitor.width,
                                   wm_rect_bottom(&monitor) - wm_rect_bottom(&work)});
}

static void add_rect(snap_obstacles *obstacles, wm_rect rect)
{
    if (rect.width > 0 && rect.height > 0) {
        obstacles->rects[obstacles->count++] = rect;
    }
}
