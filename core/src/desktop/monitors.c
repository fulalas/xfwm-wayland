#include "monitors.h"

#include <limits.h>
#include <string.h>

#include "desktop_state.h"

typedef bool (*monitor_match)(const wm_rect *monitor, const wm_rect *wanted);

static bool find_monitor(const wm_desktop *desktop, monitor_match match, const wm_rect *wanted,
                         size_t *index);
static wm_rect spot(wm_point point);
static size_t nearest(const wm_desktop *desktop, wm_point point);
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
    size_t index;
    wm_rect around = spot(point);
    return find_monitor(desktop, wm_rect_intersects, &around, &index) ? index
                                                                      : nearest(desktop, point);
}

size_t monitors_index_of(const wm_desktop *desktop, const wm_window *window)
{
    return monitors_index_at(desktop, wm_rect_center(&window->client));
}

bool monitors_cover(const wm_desktop *desktop, wm_point point)
{
    size_t index;
    wm_rect around = spot(point);
    return find_monitor(desktop, wm_rect_intersects, &around, &index);
}

bool monitors_overlap(const wm_desktop *desktop, const wm_rect *full)
{
    size_t index;
    return find_monitor(desktop, wm_rect_intersects, full, &index);
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

void monitors_remember(wm_desktop *desktop)
{
    monitor_memory *memory = &desktop->remembered_monitors;
    size_t count = monitors_count(desktop);
    memory->count = count < MONITORS_REMEMBERED_MAX ? count : MONITORS_REMEMBERED_MAX;
    for (size_t i = 0; i < memory->count; i++) {
        memory->full[i] = monitors_full_rect(desktop, i);
        memory->work[i] = monitors_work_area_of(desktop, i);
    }
}

bool monitors_remembered_at(const wm_desktop *desktop, wm_point point, wm_rect *full,
                            wm_rect *work)
{
    const monitor_memory *memory = &desktop->remembered_monitors;
    for (size_t i = 0; i < memory->count; i++) {
        if (wm_rect_contains(&memory->full[i], point)) {
            *full = memory->full[i];
            *work = memory->work[i];
            return true;
        }
    }
    return false;
}

bool monitors_find(const wm_desktop *desktop, const wm_rect *full, size_t *index)
{
    return find_monitor(desktop, wm_rect_equals, full, index);
}

bool monitors_find_named(const wm_desktop *desktop, const char *name, size_t *index)
{
    size_t count = monitors_count(desktop);
    for (size_t i = 0; i < count; i++) {
        const char *output_name = output_at(desktop, i).name;
        if (output_name != NULL && strcmp(output_name, name) == 0) {
            *index = i;
            return true;
        }
    }
    return false;
}

static bool find_monitor(const wm_desktop *desktop, monitor_match match, const wm_rect *wanted,
                         size_t *index)
{
    size_t count = monitors_count(desktop);
    for (size_t i = 0; i < count; i++) {
        wm_rect full = monitors_full_rect(desktop, i);
        if (match(&full, wanted)) {
            *index = i;
            return true;
        }
    }
    return false;
}

static wm_rect spot(wm_point point)
{
    return (wm_rect) {point.x, point.y, 1, 1};
}

static size_t nearest(const wm_desktop *desktop, wm_point point)
{
    size_t count = monitors_count(desktop);
    size_t found = 0;
    long best = LONG_MAX;
    for (size_t i = 0; i < count; i++) {
        wm_rect full = monitors_full_rect(desktop, i);
        long distance = distance_squared(&full, point);
        if (distance < best) {
            best = distance;
            found = i;
        }
    }
    return found;
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
