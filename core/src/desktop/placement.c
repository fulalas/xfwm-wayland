#include "placement.h"

#include <stdlib.h>

#include "desktop_state.h"
#include "frame.h"
#include "maximize.h"
#include "monitors.h"

enum {
    PERCENT = 100,
    CENTERED_TOLERANCE = 25,
    ANCHOR_COLUMNS = 3,
};

typedef enum anchor_align {
    ALIGN_START,
    ALIGN_MIDDLE,
    ALIGN_END,
} anchor_align;

static bool is_placed_by_client(const wm_window *window);
static void position_given(wm_desktop *desktop, wm_window *window);
static void keep_visible(wm_desktop *desktop, wm_window *window, const wm_rect *monitor);
static bool is_centered_dialog(const wm_desktop *desktop, const wm_window *window);
static void place(wm_desktop *desktop, wm_window *window, const wm_rect *area);
static bool below_ratio(const wm_desktop *desktop, const wm_window *window, const wm_rect *area);
static void center_placement(const wm_desktop *desktop, wm_window *window, const wm_rect *area);
static void mouse_placement(const wm_desktop *desktop, wm_window *window, const wm_rect *area);
static void auto_maximize(wm_desktop *desktop, wm_window *window, const wm_rect *area);
static int align(int start, int length, int size, anchor_align part, int margin);

void placement_init_position(wm_desktop *desktop, wm_window *window)
{
    wm_point pointer = monitors_pointer(desktop);
    wm_rect area = monitors_work_area_at(desktop, pointer);
    if (is_placed_by_client(window)) {
        position_given(desktop, window);
        area = monitors_work_area_at(desktop, frame_center(desktop, window));
    } else {
        place(desktop, window, &area);
    }
    if (window_is_regular_focusable(window)) {
        auto_maximize(desktop, window, &area);
    }
}

void placement_place_in(wm_desktop *desktop, wm_window *window, const wm_rect *area)
{
    place(desktop, window, area);
}

void placement_anchor(const wm_desktop *desktop, wm_window *window, const wm_rect *area,
                      wm_rule_anchor anchor, int margin)
{
    if (anchor == WM_ANCHOR_NONE) {
        return;
    }
    int cell = (int) anchor - WM_ANCHOR_TOP_LEFT;
    wm_extents extents = frame_extents(desktop, window);
    wm_rect frame = frame_rect(desktop, window);
    anchor_align column = (anchor_align) (cell % ANCHOR_COLUMNS);
    anchor_align row = (anchor_align) (cell / ANCHOR_COLUMNS);
    window->client.x = align(area->x, area->width, frame.width, column, margin) + extents.left;
    window->client.y = align(area->y, area->height, frame.height, row, margin) + extents.top;
}

static bool is_placed_by_client(const wm_window *window)
{
    return window->has_position || window->parent != NULL || window_is_dont_place(window)
           || window_is_dialog(window);
}

static void position_given(wm_desktop *desktop, wm_window *window)
{
    const wm_window *parent = window->parent;
    if (!window->has_position && parent != NULL) {
        window->client.x = parent->client.x + (parent->client.width - window->client.width) / 2;
        window->client.y = parent->client.y + (parent->client.height - window->client.height) / 2;
    }
    wm_point anchor = monitors_pointer(desktop);
    if (window->has_position && monitors_count(desktop) > 1) {
        anchor = frame_center(desktop, window);
    }
    wm_rect monitor = monitors_full_at(desktop, anchor);
    if (window_is_constrained(window)) {
        keep_visible(desktop, window, &monitor);
    }
}

static void keep_visible(wm_desktop *desktop, wm_window *window, const wm_rect *monitor)
{
    if (is_centered_dialog(desktop, window)) {
        window->client.x = monitor->x + (monitor->width - window->client.width) / 2;
        window->client.y = monitor->y + (monitor->height - window->client.height) / 2;
    }
    placement_constrain_fully(desktop, window);
}

static bool is_centered_dialog(const wm_desktop *desktop, const wm_window *window)
{
    if (!window_is_dialog(window)) {
        return false;
    }
    if (!window->has_position) {
        return window->parent == NULL;
    }
    if (window->client.x == 0 && window->client.y == 0) {
        return true;
    }
    if (monitors_count(desktop) < 2) {
        return false;
    }
    wm_rect screen = monitors_screen_bounds(desktop);
    int dx = abs(window->client.x - (screen.width - window->client.width) / 2);
    int dy = abs(window->client.y - (screen.height - window->client.height) / 2);
    return dx < CENTERED_TOLERANCE && dy < CENTERED_TOLERANCE;
}

static void place(wm_desktop *desktop, wm_window *window, const wm_rect *area)
{
    wm_rect frame = frame_rect(desktop, window);
    if (below_ratio(desktop, window, area)) {
        if (desktop->config.placement.mode == WM_PLACE_MOUSE) {
            mouse_placement(desktop, window, area);
        } else {
            center_placement(desktop, window, area);
        }
    } else if (frame.width >= area->width && frame.height >= area->height) {
        center_placement(desktop, window, area);
    } else {
        placement_smart(desktop, window, area);
    }
}

static bool below_ratio(const wm_desktop *desktop, const wm_window *window, const wm_rect *area)
{
    int ratio = desktop->config.placement.ratio_percent;
    wm_rect frame = frame_rect(desktop, window);
    long long frame_area = (long long) frame.width * frame.height;
    long long full_area = (long long) area->width * area->height;
    return ratio >= PERCENT || (long long) PERCENT * frame_area < (long long) ratio * full_area;
}

static void center_placement(const wm_desktop *desktop, wm_window *window, const wm_rect *area)
{
    placement_anchor(desktop, window, area, WM_ANCHOR_CENTER, 0);
}

static void mouse_placement(const wm_desktop *desktop, wm_window *window, const wm_rect *area)
{
    wm_point pointer = monitors_pointer(desktop);
    wm_extents extents = frame_extents(desktop, window);
    wm_rect frame = frame_rect(desktop, window);
    int x = pointer.x + extents.left - frame.width / 2;
    int y = pointer.y + extents.top - frame.height / 2;
    x = wm_min(x, area->x + area->width - frame.width + extents.left);
    y = wm_min(y, area->y + area->height - frame.height + extents.top);
    window->client.x = wm_max(x, area->x + extents.left);
    window->client.y = wm_max(y, area->y + extents.top);
}

static void auto_maximize(wm_desktop *desktop, wm_window *window, const wm_rect *area)
{
    if (window_is_fullscreen(window) || !window_has_flag(window, WINDOW_HAS_BORDER)) {
        return;
    }
    wm_rect frame = frame_rect(desktop, window);
    if (frame.width >= area->width && frame.height >= area->height) {
        maximize_save_geometry(window);
        window->states |= WM_STATE_MAXIMIZED;
        window->restore_size_pos = true;
    }
}

static int align(int start, int length, int size, anchor_align part, int margin)
{
    int position = start + margin;
    if (part == ALIGN_MIDDLE) {
        position = start + (length - size) / 2;
    } else if (part == ALIGN_END) {
        position = start + length - size - margin;
    }
    return wm_max(position, start);
}
