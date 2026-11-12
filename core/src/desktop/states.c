#include "states.h"

#include "desktop_state.h"
#include "frame.h"
#include "monitors.h"
#include "moveresize.h"
#include "placement.h"
#include "stack.h"
#include "transients.h"
#include "visibility.h"

static void set_sticky(wm_desktop *desktop, wm_window *window, uint32_t sticky);
static bool can_change_layer(const wm_window *window);
static void set_layer_states(wm_desktop *desktop, wm_window *window, uint32_t states);
static bool fits_fullscreen(const wm_desktop *desktop, const wm_window *window);

void states_shade(wm_desktop *desktop, wm_window *window)
{
    if (!frame_is_shown(desktop, window) || window_is_shaded(window)) {
        return;
    }
    window->states |= WM_STATE_SHADED;
    placement_constrain(desktop, window);
    window_mark_dirty(window);
}

void states_unshade(wm_desktop *desktop, wm_window *window)
{
    (void) desktop;
    if (!window_is_shaded(window)) {
        return;
    }
    window->states &= ~(uint32_t) WM_STATE_SHADED;
    window_mark_dirty(window);
}

void states_toggle_shade(wm_desktop *desktop, wm_window *window)
{
    if (window_is_shaded(window)) {
        states_unshade(desktop, window);
    } else {
        states_shade(desktop, window);
    }
}

void states_stick(wm_desktop *desktop, wm_window *window)
{
    set_sticky(desktop, window, WM_STATE_STICKY);
}

void states_unstick(wm_desktop *desktop, wm_window *window)
{
    set_sticky(desktop, window, 0);
}

void states_toggle_stick(wm_desktop *desktop, wm_window *window)
{
    if (window_is_sticky(window)) {
        states_unstick(desktop, window);
    } else {
        states_stick(desktop, window);
    }
}

void states_toggle_above(wm_desktop *desktop, wm_window *window)
{
    if (can_change_layer(window)) {
        uint32_t above = window->states & WM_STATE_ABOVE;
        set_layer_states(desktop, window, above ? 0 : WM_STATE_ABOVE);
    }
}

void states_toggle_below(wm_desktop *desktop, wm_window *window)
{
    if (can_change_layer(window)) {
        uint32_t below = window->states & WM_STATE_BELOW;
        set_layer_states(desktop, window, below ? 0 : WM_STATE_BELOW);
    }
}

void states_set_normal_layer(wm_desktop *desktop, wm_window *window)
{
    if (can_change_layer(window)) {
        set_layer_states(desktop, window, 0);
    }
}

void states_enter_fullscreen(wm_desktop *desktop, wm_window *window)
{
    if (window_is_fullscreen(window)) {
        return;
    }
    moveresize_window_gone(desktop, window);
    window->pre_fullscreen = window->client;
    window->states |= WM_STATE_FULLSCREEN;
    window->tile = TILE_NONE;
    window_placed(window);
    states_update_fullscreen_size(desktop, window);
    stack_update_layers(desktop);
}

void states_leave_fullscreen(wm_desktop *desktop, wm_window *window)
{
    if (!window_is_fullscreen(window)) {
        return;
    }
    window->states &= ~(uint32_t) WM_STATE_FULLSCREEN;
    if (window->fullscreen_area_once) {
        window->fullscreen_area = (wm_rect) {0, 0, 0, 0};
        window->fullscreen_area_once = false;
    }
    window_placed(window);
    states_update_fullscreen_size(desktop, window);
    stack_update_layers(desktop);
}

void states_toggle_fullscreen(wm_desktop *desktop, wm_window *window)
{
    if (!window_is_fullscreen(window) && !fits_fullscreen(desktop, window)) {
        return;
    }
    if (window->parent != NULL || window->type != WM_TYPE_NORMAL || window_is_shaded(window)) {
        return;
    }
    if (window_is_fullscreen(window)) {
        states_leave_fullscreen(desktop, window);
    } else {
        states_enter_fullscreen(desktop, window);
    }
}

void states_update_fullscreen_size(wm_desktop *desktop, wm_window *window)
{
    if (window_is_fullscreen(window)) {
        wm_rect frame = frame_rect_for_client(desktop, window, &window->pre_fullscreen);
        wm_rect screen = monitors_screen_bounds(desktop);
        bool has_area = window->fullscreen_area.width > 0 && window->fullscreen_area.height > 0
                        && wm_rect_intersects(&window->fullscreen_area, &screen);
        window->client = has_area ? window->fullscreen_area
                                  : monitors_full_at(desktop, wm_rect_center(&frame));
    } else {
        window->client = window->pre_fullscreen;
    }
    window_mark_dirty(window);
}

static void set_sticky(wm_desktop *desktop, wm_window *window, uint32_t sticky)
{
    window_list group = transient_list(desktop, window);
    for (size_t i = 0; i < group.count; i++) {
        group.items[i]->states = (group.items[i]->states & ~(uint32_t) WM_STATE_STICKY) | sticky;
        window_mark_dirty(group.items[i]);
    }
    window_list_free(&group);
    visibility_set_workspace(desktop, window, desktop->workspaces.current);
}

static bool can_change_layer(const wm_window *window)
{
    return window_is_regular_focusable(window) && window->parent == NULL
           && !window_is_fullscreen(window);
}

static void set_layer_states(wm_desktop *desktop, wm_window *window, uint32_t states)
{
    window->states &= ~(uint32_t) (WM_STATE_ABOVE | WM_STATE_BELOW);
    window->states |= states;
    window_mark_dirty(window);
    stack_update_layers(desktop);
}

static bool fits_fullscreen(const wm_desktop *desktop, const wm_window *window)
{
    const wm_size *max = &window->hints.max;
    if (max->width <= 0 || max->height <= 0) {
        return true;
    }
    wm_rect monitor = monitors_full_at(desktop, frame_center(desktop, window));
    return max->width >= monitor.width && max->height >= monitor.height;
}
