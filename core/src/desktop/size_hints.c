#include "size_hints.h"

#include "desktop_state.h"

typedef struct size_limits {
    int min;
    int max;
} size_limits;

static int check_size(const wm_desktop *desktop, const wm_window *window, int size,
                      size_limits limits);
static bool bypasses_max(const wm_desktop *desktop, const wm_window *window);

int size_hints_check_width(const wm_desktop *desktop, const wm_window *window, int width)
{
    size_limits limits = {window->hints.min.width, window->hints.max.width};
    return check_size(desktop, window, width, limits);
}

int size_hints_check_height(const wm_desktop *desktop, const wm_window *window, int height)
{
    size_limits limits = {window->hints.min.height, window->hints.max.height};
    return check_size(desktop, window, height, limits);
}

bool size_hints_allow(const wm_window *window, wm_size size)
{
    const wm_size_hints *hints = &window->hints;
    if (size.width < hints->min.width || size.height < hints->min.height) {
        return false;
    }
    if (hints->max.width > 0 && size.width > hints->max.width) {
        return false;
    }
    return hints->max.height <= 0 || size.height <= hints->max.height;
}

static int check_size(const wm_desktop *desktop, const wm_window *window, int size,
                      size_limits limits)
{
    if (!bypasses_max(desktop, window) && limits.max > 0 && size > limits.max) {
        size = limits.max;
    }
    if (size < limits.min) {
        size = limits.min;
    }
    return size < 1 ? 1 : size;
}

static bool bypasses_max(const wm_desktop *desktop, const wm_window *window)
{
    return window_is_fullscreen(window)
           || (window_is_maximized(window) && desktop->config.frame.borderless_maximize);
}
