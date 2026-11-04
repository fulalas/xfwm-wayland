#include "desktop_state.h"
#include "present.h"
#include "stack.h"
#include "states.h"
#include "urgency.h"
#include "window_ops.h"

typedef void (*state_handler)(wm_desktop *desktop, wm_window *window);

typedef struct state_rule {
    uint32_t state;
    state_handler set;
    state_handler unset;
} state_rule;

static bool only_minimizes(const wm_state_change *change);
static void request_maximize(wm_desktop *desktop, wm_window *window, const wm_state_change *change);
static void apply_rules(wm_desktop *desktop, wm_window *window, const wm_state_change *change);
static void apply_plain_flags(wm_desktop *desktop, wm_window *window,
                              const wm_state_change *change);
static void set_above(wm_desktop *desktop, wm_window *window);
static void unset_above(wm_desktop *desktop, wm_window *window);
static void set_below(wm_desktop *desktop, wm_window *window);
static void unset_below(wm_desktop *desktop, wm_window *window);
static void set_sticky(wm_desktop *desktop, wm_window *window);
static void unset_sticky(wm_desktop *desktop, wm_window *window);

static const state_rule RULES[] = {
    {WM_STATE_FULLSCREEN, states_enter_fullscreen, states_leave_fullscreen},
    {WM_STATE_MINIMIZED, window_ops_hide, window_ops_show},
    {WM_STATE_SHADED, states_shade, states_unshade},
    {WM_STATE_STICKY, set_sticky, unset_sticky},
    {WM_STATE_ABOVE, set_above, unset_above},
    {WM_STATE_BELOW, set_below, unset_below},
    {WM_STATE_URGENT, urgency_set, urgency_clear},
};

void wm_request_state(wm_desktop *desktop, wm_window *window, const wm_state_change *change)
{
    request_maximize(desktop, window, change);
    apply_rules(desktop, window, change);
    apply_plain_flags(desktop, window, change);
    if (!only_minimizes(change)) {
        present_request_configure(window);
    }
    present_flush(desktop);
}

static bool only_minimizes(const wm_state_change *change)
{
    return change->set == WM_STATE_MINIMIZED && change->unset == 0;
}

static void request_maximize(wm_desktop *desktop, wm_window *window, const wm_state_change *change)
{
    uint32_t current = window->states & WM_STATE_MAXIMIZED;
    uint32_t wanted = ((current | change->set) & ~change->unset) & WM_STATE_MAXIMIZED;
    if (wanted != current) {
        window_ops_maximize(desktop, window, wanted ^ current);
    }
}

static void apply_rules(wm_desktop *desktop, wm_window *window, const wm_state_change *change)
{
    for (size_t i = 0; i < sizeof(RULES) / sizeof(RULES[0]); i++) {
        if (change->set & RULES[i].state) {
            RULES[i].set(desktop, window);
        } else if (change->unset & RULES[i].state) {
            RULES[i].unset(desktop, window);
        }
    }
}

static void apply_plain_flags(wm_desktop *desktop, wm_window *window,
                              const wm_state_change *change)
{
    uint32_t plain = WM_STATE_SKIP_TASKBAR | WM_STATE_SKIP_PAGER | WM_STATE_MODAL
                     | WM_STATE_HIDE_TITLEBAR;
    uint32_t before = window->states;
    window->states |= change->set & plain;
    window->states &= ~(change->unset & plain);
    if (window->states != before) {
        window_mark_dirty(window);
        stack_update_layers(desktop);
    }
}

static void set_above(wm_desktop *desktop, wm_window *window)
{
    if (!window_has_state(window, WM_STATE_ABOVE)) {
        states_toggle_above(desktop, window);
    }
}

static void unset_above(wm_desktop *desktop, wm_window *window)
{
    if (window_has_state(window, WM_STATE_ABOVE)) {
        states_set_normal_layer(desktop, window);
    }
}

static void set_below(wm_desktop *desktop, wm_window *window)
{
    if (!window_has_state(window, WM_STATE_BELOW)) {
        states_toggle_below(desktop, window);
    }
}

static void unset_below(wm_desktop *desktop, wm_window *window)
{
    if (window_has_state(window, WM_STATE_BELOW)) {
        states_set_normal_layer(desktop, window);
    }
}

static void set_sticky(wm_desktop *desktop, wm_window *window)
{
    if (window_has_flag(window, WINDOW_HAS_STICK) && !window_is_sticky(window)) {
        states_stick(desktop, window);
    }
}

static void unset_sticky(wm_desktop *desktop, wm_window *window)
{
    if (window_is_sticky(window)) {
        states_unstick(desktop, window);
    }
}
