#include "monitors.h"

#include <limits.h>

#include "desktop_state.h"

static wm_output_info output_at(const wm_desktop *desktop, size_t index);
static long distance_squared(const wm_rect *rect, wm_point point);
static wm_rect margin_box(const wm_desktop *desktop);

size_t monitors_count(const wm_desktop *desktop)
{
    return desktop->ports.environment.output_count(desktop->ports.environment.self);
}

wm_rect monitors_full_rect(const wm_desktop *desktop, size_t index)
{
    return output_at(desktop, index).full;
}

size_t monitors_index_at(const wm_desktop *desktop, wm_point point)
{
    size_t count = monitors_count(desktop);
    size_t nearest = 0;
    long best = LONG_MAX;
    for (size_t i = 0; i < count; i++) {
        wm_rect full = monitors_full_rect(desktop, i);
        if (wm_rect_contains(&full, point)) {
            return i;
        }
        long distance = distance_squared(&full, point);
        if (distance < best) {
            best = distance;
            nearest = i;
        }
    }
    return nearest;
}

wm_rect monitors_full_at(const wm_desktop *desktop, wm_point point)
{
    if (monitors_count(desktop) == 0) {
        return (wm_rect) {0, 0, 0, 0};
    }
    return monitors_full_rect(desktop, monitors_index_at(desktop, point));
}

bool monitors_primary_full(const wm_desktop *desktop, wm_rect *full)
{
    size_t count = monitors_count(desktop);
    for (size_t i = 0; i < count; i++) {
        wm_output_info info = output_at(desktop, i);
        if (info.primary) {
            *full = info.full;
            return true;
        }
    }
    return false;
}

wm_rect monitors_work_area_at(const wm_desktop *desktop, wm_point point)
{
    if (monitors_count(desktop) == 0) {
        return (wm_rect) {0, 0, 0, 0};
    }
    return monitors_work_area_of(desktop, monitors_index_at(desktop, point));
}

wm_rect monitors_work_area_of(const wm_desktop *desktop, size_t index)
{
    wm_rect usable = output_at(desktop, index).usable;
    wm_rect box = margin_box(desktop);
    return wm_rect_intersection(&usable, &box);
}

wm_rect monitors_screen_bounds(const wm_desktop *desktop)
{
    size_t count = monitors_count(desktop);
    wm_rect bounds = {0, 0, 0, 0};
    for (size_t i = 0; i < count; i++) {
        wm_rect full = monitors_full_rect(desktop, i);
        bounds = i == 0 ? full : wm_rect_union(&bounds, &full);
    }
    return bounds;
}

wm_point monitors_pointer(const wm_desktop *desktop)
{
    return desktop->ports.environment.pointer_position(desktop->ports.environment.self);
}

static wm_output_info output_at(const wm_desktop *desktop, size_t index)
{
    return desktop->ports.environment.output_at_index(desktop->ports.environment.self, index);
}

static long distance_squared(const wm_rect *rect, wm_point point)
{
    wm_point center = wm_rect_center(rect);
    long dx = point.x - center.x;
    long dy = point.y - center.y;
    return dx * dx + dy * dy;
}

static wm_rect margin_box(const wm_desktop *desktop)
{
    const wm_margins *margins = &desktop->config.placement.margins;
    wm_rect bounds = monitors_screen_bounds(desktop);
    wm_extents extents = {margins->left, margins->right, margins->top, margins->bottom};
    return wm_rect_shrink(&bounds, &extents);
}
