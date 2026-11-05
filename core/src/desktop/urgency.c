#include "urgency.h"

#include "desktop_state.h"

enum {
    BLINK_TIMEOUT_MS = 500,
    MAX_BLINK_ITERATIONS = 5,
    BLINK_TOGGLES = 2 * MAX_BLINK_ITERATIONS,
};

static void toggle_look(wm_desktop *desktop, wm_window *window);
static void keep_inactive_look(wm_desktop *desktop, wm_window *window);
static void restart(wm_desktop *desktop, wm_window *window);

void urgency_set(wm_desktop *desktop, wm_window *window)
{
    if (desktop->focus.focused == window) {
        return;
    }
    window->states |= WM_STATE_URGENT;
    window_mark_dirty(window);
    urgency_refresh(desktop, window);
}

void urgency_clear(wm_desktop *desktop, wm_window *window)
{
    if (!window_has_state(window, WM_STATE_URGENT)) {
        return;
    }
    window->states &= ~(uint32_t) WM_STATE_URGENT;
    window->seen_active = false;
    window->blink_iterations = 0;
    timers_cancel(desktop, TIMER_BLINK, window);
    window_mark_dirty(window);
}

void urgency_refresh(wm_desktop *desktop, wm_window *window)
{
    if (!window->visible) {
        timers_cancel(desktop, TIMER_BLINK, window);
        window->seen_active = false;
        window->blink_iterations = 0;
        window_mark_dirty(window);
        return;
    }
    if (window_has_state(window, WM_STATE_URGENT)
        && !timers_is_running(desktop, TIMER_BLINK, window)) {
        restart(desktop, window);
    }
}

void urgency_on_blink(wm_desktop *desktop, wm_window *window)
{
    if (window == desktop->focus.focused) {
        window->blink_iterations = 0;
        return;
    }
    if (!desktop->config.focus.urgent_blink) {
        keep_inactive_look(desktop, window);
        return;
    }
    if (window->blink_iterations < BLINK_TOGGLES) {
        window->blink_iterations++;
        toggle_look(desktop, window);
        return;
    }
    if (desktop->config.focus.repeat_urgent_blink) {
        window->blink_iterations = 1;
        toggle_look(desktop, window);
        return;
    }
    window->seen_active = true;
    window_mark_dirty(window);
}

static void toggle_look(wm_desktop *desktop, wm_window *window)
{
    window->seen_active = !window->seen_active;
    window_mark_dirty(window);
    restart(desktop, window);
}

static void keep_inactive_look(wm_desktop *desktop, wm_window *window)
{
    if (window->seen_active) {
        window->seen_active = false;
        window_mark_dirty(window);
    }
    window->blink_iterations = 0;
    restart(desktop, window);
}

static void restart(wm_desktop *desktop, wm_window *window)
{
    timers_start(desktop, TIMER_BLINK, window, BLINK_TIMEOUT_MS);
}
