#include "size_hints.h"

#include "desktop_state.h"

typedef struct size_limits {
    int min;
    int max;
    int base;
    int step;
} size_limits;

typedef struct ratio_side {
    long long num;
    long long den;
    int *grow;
    int *shrink;
    int grow_step;
    int shrink_step;
    int grow_max;
    int shrink_min;
} ratio_side;

static size_limits width_limits(const wm_size_hints *hints);
static size_limits height_limits(const wm_size_hints *hints);
static int check_size(const wm_desktop *desktop, const wm_window *window, int size,
                      size_limits limits, bool by_user);
static void keep_ratio(const wm_size_hints *hints, uint32_t edges, wm_rect *client);
static void fix_ratio(const ratio_side *side, bool from_side);
static bool off_ratio(const ratio_side *side);
static void grow_to_ratio(const ratio_side *side);
static int multiple(long long value, int step);
static bool fits_max(int max, int size);
static int key_step(int step, int nominal);
static bool bypasses_max(const wm_desktop *desktop, const wm_window *window);

void size_hints_normalize(wm_size_hints *hints)
{
    hints->step.width = wm_max(hints->step.width, 1);
    hints->step.height = wm_max(hints->step.height, 1);
}

int size_hints_check_width(const wm_desktop *desktop, const wm_window *window, int width)
{
    return check_size(desktop, window, width, width_limits(&window->hints), false);
}

int size_hints_check_height(const wm_desktop *desktop, const wm_window *window, int height)
{
    return check_size(desktop, window, height, height_limits(&window->hints), false);
}

void size_hints_fit_user(const wm_desktop *desktop, const wm_window *window, uint32_t edges,
                         wm_rect *client)
{
    const wm_size_hints *hints = &window->hints;
    keep_ratio(hints, edges, client);
    client->width = check_size(desktop, window, client->width, width_limits(hints), true);
    client->height = check_size(desktop, window, client->height, height_limits(hints), true);
}

bool size_hints_allow(const wm_window *window, wm_size size)
{
    const wm_size_hints *hints = &window->hints;
    if (size.width < hints->min.width || size.height < hints->min.height) {
        return false;
    }
    return fits_max(hints->max.width, size.width) && fits_max(hints->max.height, size.height);
}

bool size_hints_has_steps(const wm_window *window)
{
    return window->hints.step.width > 1 || window->hints.step.height > 1;
}

wm_size size_hints_steps_of(const wm_window *window)
{
    const wm_size_hints *hints = &window->hints;
    int columns = (window->client.width - hints->base.width) / hints->step.width;
    int rows = (window->client.height - hints->base.height) / hints->step.height;
    return (wm_size) {wm_max(columns, 0), wm_max(rows, 0)};
}

wm_point size_hints_key_steps(const wm_window *window, int nominal)
{
    const wm_size_hints *hints = &window->hints;
    return (wm_point) {key_step(hints->step.width, nominal), key_step(hints->step.height, nominal)};
}

static size_limits width_limits(const wm_size_hints *hints)
{
    return (size_limits) {hints->min.width, hints->max.width, hints->base.width,
                          hints->step.width};
}

static size_limits height_limits(const wm_size_hints *hints)
{
    return (size_limits) {hints->min.height, hints->max.height, hints->base.height,
                          hints->step.height};
}

static int check_size(const wm_desktop *desktop, const wm_window *window, int size,
                      size_limits limits, bool by_user)
{
    if (!bypasses_max(desktop, window)) {
        if (by_user) {
            size = limits.base + multiple(size - limits.base, limits.step);
        }
        if (!fits_max(limits.max, size)) {
            size = limits.max;
        }
    }
    if (size < limits.min) {
        size = limits.min;
    }
    return size < 1 ? 1 : size;
}

static void keep_ratio(const wm_size_hints *hints, uint32_t edges, wm_rect *client)
{
    if (hints->min_aspect.width <= 0) {
        return;
    }
    ratio_side widen = {hints->min_aspect.width, hints->min_aspect.height, &client->width,
                        &client->height, hints->step.width, hints->step.height,
                        hints->max.width, wm_max(hints->min.height, 1)};
    ratio_side narrow = {hints->max_aspect.height, hints->max_aspect.width, &client->height,
                         &client->width, hints->step.height, hints->step.width,
                         hints->max.height, wm_max(hints->min.width, 1)};
    fix_ratio(&widen, edges == WM_EDGE_TOP || edges == WM_EDGE_BOTTOM);
    fix_ratio(&narrow, edges == WM_EDGE_LEFT || edges == WM_EDGE_RIGHT);
}

static void fix_ratio(const ratio_side *side, bool from_side)
{
    if (!off_ratio(side)) {
        return;
    }
    if (from_side) {
        grow_to_ratio(side);
    }
    if (!off_ratio(side)) {
        return;
    }
    int delta = multiple(*side->shrink - *side->grow * side->den / side->num, side->shrink_step);
    if (*side->shrink - delta >= side->shrink_min) {
        *side->shrink -= delta;
        return;
    }
    grow_to_ratio(side);
}

static bool off_ratio(const ratio_side *side)
{
    return side->num * *side->shrink > side->den * *side->grow;
}

static void grow_to_ratio(const ratio_side *side)
{
    int delta = multiple(side->num * *side->shrink / side->den - *side->grow, side->grow_step);
    if (fits_max(side->grow_max, *side->grow + delta)) {
        *side->grow += delta;
    }
}

static int multiple(long long value, int step)
{
    return (int) (value / step * step);
}

static bool fits_max(int max, int size)
{
    return max <= 0 || size <= max;
}

static int key_step(int step, int nominal)
{
    return wm_max(nominal / step, 1) * step;
}

static bool bypasses_max(const wm_desktop *desktop, const wm_window *window)
{
    return window_is_fullscreen(window)
           || (window_is_maximized(window) && desktop->config.frame.borderless_maximize);
}
