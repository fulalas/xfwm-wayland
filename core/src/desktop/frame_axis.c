#include "frame_input.h"

#include "desktop_state.h"
#include "effects.h"
#include "states.h"
#include "workspaces.h"
#include "zoom.h"

enum {
    SCROLL_DROP_MS = 25,
};

static bool zoom_scroll(wm_desktop *desktop, const wm_axis_event *event);
static void title_scroll(wm_desktop *desktop, wm_window *window, const wm_axis_event *event);
static void opacity_scroll(wm_desktop *desktop, wm_window *window, int steps);
static void rollup_scroll(wm_desktop *desktop, wm_window *window, int steps);
static wm_dispatch desktop_scroll(wm_desktop *desktop, const wm_axis_event *event);

wm_dispatch frame_input_axis(wm_desktop *desktop, const wm_axis_event *event)
{
    if (zoom_scroll(desktop, event)) {
        return WM_CONSUMED;
    }
    const wm_pointer_target *target = &event->pointer.target;
    if (target->kind == WM_TARGET_DESKTOP) {
        return desktop_scroll(desktop, event);
    }
    if (target->kind != WM_TARGET_FRAME || target->window == NULL) {
        return WM_FORWARD;
    }
    if (frame_input_part_at(target->window, event->pointer.position) == WM_PART_TITLE) {
        title_scroll(desktop, target->window, event);
    }
    return WM_CONSUMED;
}

static bool zoom_scroll(wm_desktop *desktop, const wm_axis_event *event)
{
    if (!desktop->config.mouse.zoom_desktop || !effects_enabled(desktop)
        || !frame_input_is_easy_click(desktop, event->pointer.modifiers)
        || event->vertical_steps == 0) {
        return false;
    }
    int steps = event->vertical_steps;
    for (; steps < 0; steps++) {
        zoom_in(desktop, event->pointer.position);
    }
    for (; steps > 0; steps--) {
        zoom_out(desktop, event->pointer.position);
    }
    return true;
}

static void title_scroll(wm_desktop *desktop, wm_window *window, const wm_axis_event *event)
{
    bool easy = frame_input_is_easy_click(desktop, event->pointer.modifiers)
                && effects_enabled(desktop);
    if (event->vertical_steps != 0 && easy) {
        opacity_scroll(desktop, window, -event->vertical_steps);
    } else if (event->vertical_steps != 0) {
        rollup_scroll(desktop, window, event->vertical_steps);
    } else if (desktop->config.mouse.horiz_scroll_opacity) {
        opacity_scroll(desktop, window, event->horizontal_steps);
    }
}

static void opacity_scroll(wm_desktop *desktop, wm_window *window, int steps)
{
    if (steps > 0) {
        effects_increase_opacity(desktop, window);
    } else if (steps < 0) {
        effects_decrease_opacity(desktop, window);
    }
}

static void rollup_scroll(wm_desktop *desktop, wm_window *window, int steps)
{
    if (!desktop->config.mouse.mousewheel_rollup) {
        return;
    }
    if (steps < 0) {
        states_shade(desktop, window);
    } else {
        states_unshade(desktop, window);
    }
}

static wm_dispatch desktop_scroll(wm_desktop *desktop, const wm_axis_event *event)
{
    if (!desktop->config.mouse.scroll_workspaces || event->vertical_steps == 0) {
        return WM_FORWARD;
    }
    uint32_t time_ms = event->pointer.time_ms;
    if (desktop->has_scrolled && time_ms - desktop->last_scroll_ms < SCROLL_DROP_MS) {
        return WM_CONSUMED;
    }
    desktop->has_scrolled = true;
    desktop->last_scroll_ms = time_ms;
    int delta = event->vertical_steps < 0 ? -1 : 1;
    workspaces_switch(desktop, desktop->workspaces.current + delta, NULL);
    return WM_CONSUMED;
}
