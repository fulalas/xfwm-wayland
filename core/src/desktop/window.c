#include "window.h"

#include <stdlib.h>
#include <string.h>

static const uint32_t OPACITY_OPAQUE = 0xffffffffu;

static char *copy_text(const char *text);
static uint32_t type_removed_flags(wm_window_type type);
static uint32_t capability_flags(uint32_t capabilities);
static bool hints_allow_resize(const wm_size_hints *hints);
static bool dimension_resizable(int min, int max);
static bool window_has_all_flags(const wm_window *window, uint32_t flags);
static bool window_has_all_states(const wm_window *window, uint32_t states);

wm_window *window_create(const wm_window_desc *desc, unsigned long serial)
{
    wm_window *window = calloc(1, sizeof(*window));
    if (window == NULL) {
        return NULL;
    }
    window->handle = desc->handle;
    window->requested_type = desc->type;
    window->capabilities = desc->capabilities;
    window->states = desc->states & ~(uint32_t) WM_STATE_URGENT;
    window->serial = serial;
    window->client = desc->geometry;
    window->hints = desc->size_hints;
    window->title = copy_text(desc->title);
    window->app_id = copy_text(desc->app_id);
    window->opacity = window_opacity_from_double(desc->opacity);
    window->layer = WM_LAYER_NORMAL;
    window->activation_allowed = desc->activation_allowed;
    window->has_position = desc->has_position;
    window->dirty = true;
    return window;
}

void window_destroy(wm_window *window)
{
    free(window->title);
    free(window->app_id);
    free(window);
}

void window_set_title(wm_window *window, const char *title)
{
    free(window->title);
    window->title = copy_text(title);
    window->title_measured = false;
    window->dirty = true;
}

void window_set_app_id(wm_window *window, const char *app_id)
{
    free(window->app_id);
    window->app_id = copy_text(app_id);
    window->dirty = true;
}

void window_update_flags(wm_window *window)
{
    uint32_t flags = capability_flags(window->capabilities) | WINDOW_HAS_MENU | WINDOW_HAS_STICK;
    if (hints_allow_resize(&window->hints)) {
        flags |= WINDOW_IS_RESIZABLE;
    }
    if (window->parent != NULL) {
        flags &= ~(uint32_t) WINDOW_HAS_STICK;
    }
    window->flags = flags & ~type_removed_flags(window->type);
    window->dirty = true;
}

uint32_t window_opacity_from_double(double opacity)
{
    if (!(opacity >= 0.0) || opacity >= 1.0) {
        return OPACITY_OPAQUE;
    }
    return (uint32_t) (opacity * (double) OPACITY_OPAQUE);
}

bool window_has_state(const wm_window *window, uint32_t states)
{
    return (window->states & states) != 0;
}

bool window_has_flag(const wm_window *window, uint32_t flags)
{
    return (window->flags & flags) != 0;
}

bool window_is_maximized(const wm_window *window)
{
    return window_has_all_states(window, WM_STATE_MAXIMIZED);
}

bool window_is_minimized(const wm_window *window)
{
    return window_has_state(window, WM_STATE_MINIMIZED);
}

bool window_is_fullscreen(const wm_window *window)
{
    return window_has_state(window, WM_STATE_FULLSCREEN);
}

bool window_is_shaded(const wm_window *window)
{
    return window_has_state(window, WM_STATE_SHADED);
}

bool window_is_sticky(const wm_window *window)
{
    return window_has_state(window, WM_STATE_STICKY);
}

bool window_is_regular_focusable(const wm_window *window)
{
    return window->type == WM_TYPE_NORMAL || window->type == WM_TYPE_DIALOG
           || window->type == WM_TYPE_UTILITY;
}

bool window_is_dont_focus(const wm_window *window)
{
    return window->type == WM_TYPE_SPLASH || window->type == WM_TYPE_DOCK;
}

bool window_is_dont_place(const wm_window *window)
{
    return window->type == WM_TYPE_DESKTOP || window->type == WM_TYPE_DOCK
           || window->type == WM_TYPE_UTILITY || window->type == WM_TYPE_SPLASH;
}

bool window_is_dialog(const wm_window *window)
{
    return window->type == WM_TYPE_DIALOG;
}

bool window_can_hide(const wm_window *window)
{
    return window_has_flag(window, WINDOW_HAS_HIDE)
           && !window_has_state(window, WM_STATE_SKIP_TASKBAR);
}

bool window_can_maximize(const wm_window *window)
{
    return window_allows_maximize(window) && !window_is_fullscreen(window);
}

bool window_allows_maximize(const wm_window *window)
{
    return window_has_all_flags(window, WINDOW_HAS_MAXIMIZE | WINDOW_IS_RESIZABLE);
}

bool window_can_fill(const wm_window *window)
{
    uint32_t blocking = WM_STATE_FULLSCREEN | WM_STATE_MAXIMIZED | WM_STATE_SHADED;
    return window_has_flag(window, WINDOW_HAS_RESIZE | WINDOW_IS_RESIZABLE)
           && !window_has_state(window, blocking);
}

bool window_can_tile(const wm_window *window)
{
    return window_can_maximize(window) && !window_is_shaded(window)
           && window->type == WM_TYPE_NORMAL;
}

bool window_can_resize(const wm_window *window)
{
    return window_has_all_flags(window, WINDOW_HAS_RESIZE | WINDOW_IS_RESIZABLE);
}

bool window_is_constrained(const wm_window *window)
{
    return window->layer > WM_LAYER_DESKTOP && window->type != WM_TYPE_DESKTOP
           && window->type != WM_TYPE_DOCK;
}

void window_mark_dirty(wm_window *window)
{
    window->dirty = true;
}

static char *copy_text(const char *text)
{
    if (text == NULL) {
        return NULL;
    }
    size_t length = strlen(text) + 1;
    char *copy = malloc(length);
    if (copy != NULL) {
        memcpy(copy, text, length);
    }
    return copy;
}

static uint32_t type_removed_flags(wm_window_type type)
{
    switch (type) {
    case WM_TYPE_DESKTOP:
        return WINDOW_HAS_RESIZE | WINDOW_HAS_MOVE | WINDOW_HAS_HIDE | WINDOW_HAS_MAXIMIZE
               | WINDOW_HAS_MENU | WINDOW_HAS_STICK | WINDOW_HAS_BORDER;
    case WM_TYPE_DOCK:
        return WINDOW_HAS_BORDER | WINDOW_HAS_MOVE | WINDOW_HAS_HIDE | WINDOW_HAS_MAXIMIZE
               | WINDOW_HAS_MENU;
    case WM_TYPE_TOOLBAR:
    case WM_TYPE_MENU:
        return WINDOW_HAS_HIDE | WINDOW_HAS_MAXIMIZE;
    case WM_TYPE_SPLASH:
        return WINDOW_HAS_BORDER | WINDOW_HAS_HIDE | WINDOW_HAS_MENU | WINDOW_HAS_MOVE
               | WINDOW_HAS_RESIZE;
    default:
        return 0;
    }
}

static uint32_t capability_flags(uint32_t capabilities)
{
    static const uint32_t map[][2] = {
        {WM_CAP_DECORATED, WINDOW_HAS_BORDER}, {WM_CAP_CLOSE, WINDOW_HAS_CLOSE},
        {WM_CAP_MAXIMIZE, WINDOW_HAS_MAXIMIZE}, {WM_CAP_MINIMIZE, WINDOW_HAS_HIDE},
        {WM_CAP_RESIZE, WINDOW_HAS_RESIZE}, {WM_CAP_MOVE, WINDOW_HAS_MOVE},
        {WM_CAP_FOCUS, WINDOW_HAS_FOCUS},
    };
    uint32_t flags = 0;
    for (size_t i = 0; i < sizeof(map) / sizeof(map[0]); i++) {
        if (capabilities & map[i][0]) {
            flags |= map[i][1];
        }
    }
    return flags;
}

static bool hints_allow_resize(const wm_size_hints *hints)
{
    return dimension_resizable(hints->min.width, hints->max.width)
           || dimension_resizable(hints->min.height, hints->max.height);
}

static bool dimension_resizable(int min, int max)
{
    int effective_min = min > 1 ? min : 1;
    return max <= 0 || effective_min < max;
}

static bool window_has_all_flags(const wm_window *window, uint32_t flags)
{
    return (window->flags & flags) == flags;
}

static bool window_has_all_states(const wm_window *window, uint32_t states)
{
    return (window->states & states) == states;
}
