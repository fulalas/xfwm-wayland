#include "focus.h"

#include "desktop_state.h"
#include "frame.h"
#include "monitors.h"
#include "stack.h"
#include "timers.h"
#include "transients.h"
#include "urgency.h"
#include "visibility.h"

static void update_focus(wm_desktop *desktop, wm_window *window);
static void apply_new_focus(wm_desktop *desktop, wm_window *window);
static void raise_after_focus(wm_desktop *desktop, wm_window *window);
static bool is_excluded(const window_list *exclude, const wm_window *window);
static bool is_pass_candidate(const wm_desktop *desktop, const wm_window *window,
                              const window_list *exclude);
static wm_window *pass_target(wm_desktop *desktop, wm_window *window, const window_list *exclude);
static bool is_focus_stealing_prevented(const wm_desktop *desktop, const wm_window *window);
static bool new_window_gets_focus(const wm_desktop *desktop, const wm_window *window);
static void keep_new_window_unfocused(wm_desktop *desktop, wm_window *window);
static void keep_new_window_behind(wm_desktop *desktop, wm_window *window);
static bool frame_contains(const wm_desktop *desktop, const wm_window *window, wm_point point);
static void focus_set_ignoring_modal(wm_desktop *desktop, wm_window *window);
static wm_window *focus_top_most(const wm_desktop *desktop, wm_layer layer,
                                 const window_list *exclude);

bool focus_accepts(const wm_desktop *desktop, const wm_window *window)
{
    if (window_has_state(window, WM_STATE_MODAL)) {
        return true;
    }
    return !desktop->config.focus.focus_hint || window_has_flag(window, WINDOW_HAS_FOCUS);
}

void focus_set(wm_desktop *desktop, wm_window *window)
{
    wm_window *modal = window ? transient_modal_for(desktop, window) : NULL;
    focus_set_ignoring_modal(desktop, modal ? modal : window);
}

void focus_clear(wm_desktop *desktop)
{
    update_focus(desktop, NULL);
}

void focus_desktop_clicked(wm_desktop *desktop)
{
    timers_cancel_kind(desktop, TIMER_FOCUS);
    update_focus(desktop, NULL);
}

void focus_pass(wm_desktop *desktop, wm_window *window)
{
    wm_window *current = desktop->focus.focused;
    if (current != NULL && current != window) {
        return;
    }
    focus_set_ignoring_modal(desktop, pass_target(desktop, window, NULL));
}

void focus_pass_after_hide(wm_desktop *desktop, wm_window *window, const window_list *gone)
{
    wm_window *current = desktop->focus.focused;
    if (current != NULL && current != window && !is_excluded(gone, current)) {
        return;
    }
    wm_window *parent = window->parent;
    if (parent != NULL && is_pass_candidate(desktop, parent, gone)) {
        focus_set_ignoring_modal(desktop, parent);
        return;
    }
    focus_set_ignoring_modal(desktop, pass_target(desktop, window, gone));
}

void focus_top(wm_desktop *desktop, wm_layer layer)
{
    focus_set(desktop, focus_top_most(desktop, layer, NULL));
}

void focus_note_user_input(wm_desktop *desktop, uint32_t time_ms)
{
    desktop->has_user_input = true;
    desktop->last_user_input_ms = time_ms;
}

void focus_new_window(wm_desktop *desktop, wm_window *window)
{
    if (!new_window_gets_focus(desktop, window)) {
        if (is_focus_stealing_prevented(desktop, window)) {
            keep_new_window_behind(desktop, window);
        } else {
            keep_new_window_unfocused(desktop, window);
        }
        return;
    }
    stack_raise(desktop, window);
    visibility_unminimize(desktop, window);
    focus_set_ignoring_modal(desktop, window);
}

void focus_and_raise_on_click(wm_desktop *desktop, wm_window *window)
{
    if (!window_is_dont_focus(window)) {
        focus_set(desktop, window);
    }
    if (desktop->config.focus.raise_on_click || !window_has_flag(window, WINDOW_HAS_BORDER)) {
        timers_cancel_kind(desktop, TIMER_RAISE);
        stack_raise(desktop, window);
    }
}

void focus_pointer_entered(wm_desktop *desktop, wm_window *window)
{
    if (desktop->config.focus.click_to_focus || !focus_accepts(desktop, window)) {
        return;
    }
    timers_cancel_kind(desktop, TIMER_FOCUS);
    if (window->type == WM_TYPE_DOCK || window->type == WM_TYPE_DESKTOP) {
        return;
    }
    if (desktop->config.focus.focus_delay_ms > 0) {
        timer_request timer = {TIMER_FOCUS, window, desktop->config.focus.focus_delay_ms};
        timers_start(desktop, &timer);
        return;
    }
    focus_set(desktop, window);
}

void focus_on_delay_elapsed(wm_desktop *desktop, wm_window *window)
{
    focus_set(desktop, window);
}

void focus_on_raise_delay_elapsed(wm_desktop *desktop)
{
    if (desktop->focus.focused != NULL) {
        stack_raise(desktop, desktop->focus.focused);
    }
}

void focus_forget(wm_desktop *desktop, wm_window *window)
{
    focus_state *focus = &desktop->focus;
    if (focus->focused == window) {
        focus->focused = NULL;
    }
    if (focus->hovered == window) {
        focus->hovered = NULL;
    }
    window_list_remove(&desktop->mru, window);
}

wm_window *focus_window_at(const wm_desktop *desktop, wm_point point, const window_list *exclude)
{
    const window_list *stack = &desktop->stack;
    for (size_t i = stack->count; i > 0; i--) {
        wm_window *window = stack->items[i - 1];
        if (window->visible && window_is_regular_focusable(window)
            && !is_excluded(exclude, window) && frame_contains(desktop, window, point)) {
            return window;
        }
    }
    return NULL;
}

static void update_focus(wm_desktop *desktop, wm_window *window)
{
    wm_window *previous = desktop->focus.focused;
    if (previous == window) {
        return;
    }
    desktop->focus.focused = window;
    if (previous != NULL) {
        window_mark_dirty(previous);
    }
    if (window != NULL) {
        apply_new_focus(desktop, window);
    } else {
        stack_update_layers(desktop);
    }
}

static void apply_new_focus(wm_desktop *desktop, wm_window *window)
{
    window_list_move_to_front(&desktop->mru, window);
    window->workspace_focus = false;
    urgency_clear(desktop, window);
    window_mark_dirty(window);
    bool restacked = stack_layers_outdated(desktop);
    stack_update_layers(desktop);
    if (!restacked && desktop->config.focus.raise_on_focus) {
        raise_after_focus(desktop, window);
    }
}

static void raise_after_focus(wm_desktop *desktop, wm_window *window)
{
    if (desktop->config.focus.click_to_focus) {
        stack_raise(desktop, window);
        return;
    }
    timer_request timer = {TIMER_RAISE, NULL, desktop->config.focus.raise_delay_ms};
    timers_start(desktop, &timer);
}

static bool is_excluded(const window_list *exclude, const wm_window *window)
{
    return exclude != NULL && window_list_contains(exclude, window);
}

static bool is_pass_candidate(const wm_desktop *desktop, const wm_window *window,
                              const window_list *exclude)
{
    return window->visible && focus_accepts(desktop, window) && !window_is_dont_focus(window)
           && !is_excluded(exclude, window);
}

static wm_window *pass_target(wm_desktop *desktop, wm_window *window, const window_list *exclude)
{
    wm_layer layer = window ? window->layer : WM_LAYER_NORMAL;
    if (!desktop->config.focus.click_to_focus) {
        wm_window *under = focus_window_at(desktop, monitors_pointer(desktop), exclude);
        if (under != NULL) {
            return under;
        }
    }
    return focus_top_most(desktop, layer, exclude);
}

static bool is_focus_stealing_prevented(const wm_desktop *desktop, const wm_window *window)
{
    const wm_window *focused = desktop->focus.focused;
    if (focused == NULL || !desktop->config.focus.prevent_focus_stealing
        || !focus_accepts(desktop, window) || window_is_dont_focus(window)
        || window_has_state(window, WM_STATE_MODAL)) {
        return false;
    }
    if (focused->layer != window->layer) {
        return focused->layer > window->layer;
    }
    return focused->moving || focused->resizing || !window->activation_allowed;
}

static bool new_window_gets_focus(const wm_desktop *desktop, const wm_window *window)
{
    const wm_window *focused = desktop->focus.focused;
    if (window_has_state(window, WM_STATE_MODAL)) {
        return true;
    }
    if (!focus_accepts(desktop, window) || window_is_dont_focus(window)
        || window->skip_initial_focus || is_focus_stealing_prevented(desktop, window)) {
        return false;
    }
    if (focused != NULL && desktop->config.focus.prevent_focus_stealing
        && focused->layer < window->layer) {
        return true;
    }
    return window_is_regular_focusable(window) && desktop->config.focus.focus_new;
}

static void keep_new_window_unfocused(wm_desktop *desktop, wm_window *window)
{
    wm_window *focused = desktop->focus.focused;
    window_list_move_to_front(&desktop->mru, window);
    if (focused != NULL) {
        window_list_move_to_front(&desktop->mru, focused);
    }
    stack_raise(desktop, window);
    visibility_unminimize(desktop, window);
}

static void keep_new_window_behind(wm_desktop *desktop, wm_window *window)
{
    wm_window *focused = desktop->focus.focused;
    keep_new_window_unfocused(desktop, window);
    if (focused->layer == window->layer) {
        stack_place_below(desktop, window, focused);
    }
    urgency_set(desktop, window);
}

static bool frame_contains(const wm_desktop *desktop, const wm_window *window, wm_point point)
{
    wm_rect frame = frame_rect(desktop, window);
    return point.x >= frame.x && point.x <= wm_rect_right(&frame) && point.y >= frame.y
           && point.y <= wm_rect_bottom(&frame);
}

static void focus_set_ignoring_modal(wm_desktop *desktop, wm_window *window)
{
    if (window == NULL || !window->visible) {
        update_focus(desktop, NULL);
        return;
    }
    urgency_clear(desktop, window);
    if (window == desktop->focus.focused || !focus_accepts(desktop, window)) {
        return;
    }
    update_focus(desktop, window);
}

static wm_window *focus_top_most(const wm_desktop *desktop, wm_layer layer,
                                 const window_list *exclude)
{
    wm_window *preferred = NULL;
    wm_window *highest = NULL;
    const window_list *stack = &desktop->stack;
    for (size_t i = 0; i < stack->count; i++) {
        wm_window *window = stack->items[i];
        if (!focus_accepts(desktop, window) || window_is_dont_focus(window)
            || is_excluded(exclude, window)) {
            continue;
        }
        if (window->layer > layer) {
            break;
        }
        if (window->visible) {
            preferred = window_is_regular_focusable(window) ? window : preferred;
            highest = window;
        }
    }
    return preferred ? preferred : highest;
}
