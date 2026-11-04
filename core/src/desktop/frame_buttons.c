#include "frame_buttons.h"

#include "desktop_state.h"
#include "frame_input.h"
#include "states.h"
#include "window_ops.h"

static bool is_over_button(const wm_pointer_event *event, const wm_window *window,
                           wm_button button);
static void set_status(wm_window *window, wm_button button, wm_button_status status);
static void run_action(wm_desktop *desktop, wm_window *window, wm_button button);
static void maximize_action(wm_desktop *desktop, wm_window *window);
static void clear_hover(wm_desktop *desktop);
static wm_part hovered_part(const wm_pointer_event *event);

void frame_buttons_press(wm_desktop *desktop, wm_window *window, wm_button button,
                         uint32_t mouse_button)
{
    frame_buttons_state *state = &desktop->buttons;
    state->pressing = true;
    state->window = window;
    state->button = button;
    state->mouse_button = mouse_button;
    set_status(window, button, WM_BUTTON_PRESSED);
}

bool frame_buttons_is_pressing(const wm_desktop *desktop)
{
    return desktop->buttons.pressing;
}

wm_dispatch frame_buttons_motion(wm_desktop *desktop, const wm_pointer_event *event)
{
    frame_buttons_state *state = &desktop->buttons;
    bool over = is_over_button(event, state->window, state->button);
    set_status(state->window, state->button, over ? WM_BUTTON_PRESSED : WM_BUTTON_NORMAL);
    return WM_CONSUMED;
}

wm_dispatch frame_buttons_button(wm_desktop *desktop, const wm_button_event *event)
{
    frame_buttons_state *state = &desktop->buttons;
    if (event->pressed) {
        return WM_CONSUMED;
    }
    state->pressing = false;
    wm_window *window = state->window;
    if (window->buttons[state->button] != WM_BUTTON_PRESSED) {
        return WM_CONSUMED;
    }
    set_status(window, state->button, WM_BUTTON_HOVERED);
    desktop->buttons.hover_window = window;
    run_action(desktop, window, state->button);
    return WM_CONSUMED;
}

void frame_buttons_hover(wm_desktop *desktop, const wm_pointer_event *event)
{
    wm_window *window = event->target.kind == WM_TARGET_FRAME ? event->target.window : NULL;
    if (window != desktop->buttons.hover_window) {
        clear_hover(desktop);
    }
    if (window == NULL) {
        return;
    }
    desktop->buttons.hover_window = window;
    wm_part part = hovered_part(event);
    for (int i = 0; i < WM_BUTTON_COUNT; i++) {
        if (window->buttons[i] == WM_BUTTON_PRESSED) {
            continue;
        }
        bool hovered = part == (wm_part) (WM_PART_BUTTON_FIRST + i);
        set_status(window, (wm_button) i, hovered ? WM_BUTTON_HOVERED : WM_BUTTON_NORMAL);
    }
}

void frame_buttons_window_gone(wm_desktop *desktop, const wm_window *window)
{
    frame_buttons_state *state = &desktop->buttons;
    if (state->window == window) {
        state->pressing = false;
        state->window = NULL;
    }
    if (state->hover_window == window) {
        state->hover_window = NULL;
    }
}

static bool is_over_button(const wm_pointer_event *event, const wm_window *window,
                           wm_button button)
{
    if (event->target.kind != WM_TARGET_FRAME || event->target.window != window) {
        return false;
    }
    wm_part part = (wm_part) (WM_PART_BUTTON_FIRST + button);
    return frame_input_part_at(window, event->position) == part;
}

static void set_status(wm_window *window, wm_button button, wm_button_status status)
{
    if (window->buttons[button] != status) {
        window->buttons[button] = status;
        window_mark_dirty(window);
    }
}

static void run_action(wm_desktop *desktop, wm_window *window, wm_button button)
{
    switch (button) {
    case WM_BUTTON_HIDE:
        window_ops_hide(desktop, window);
        break;
    case WM_BUTTON_CLOSE:
        window_ops_close(desktop, window);
        break;
    case WM_BUTTON_MAXIMIZE:
        maximize_action(desktop, window);
        break;
    case WM_BUTTON_SHADE:
        states_toggle_shade(desktop, window);
        break;
    case WM_BUTTON_STICK:
        states_toggle_stick(desktop, window);
        break;
    default:
        break;
    }
}

static void maximize_action(wm_desktop *desktop, wm_window *window)
{
    switch (desktop->buttons.mouse_button) {
    case WM_MOUSE_LEFT:
        window_ops_maximize(desktop, window, WM_STATE_MAXIMIZED);
        break;
    case WM_MOUSE_MIDDLE:
        window_ops_maximize(desktop, window, WM_STATE_MAXIMIZED_VERT);
        break;
    case WM_MOUSE_RIGHT:
        window_ops_maximize(desktop, window, WM_STATE_MAXIMIZED_HORZ);
        break;
    default:
        break;
    }
}

static void clear_hover(wm_desktop *desktop)
{
    wm_window *window = desktop->buttons.hover_window;
    desktop->buttons.hover_window = NULL;
    if (window == NULL) {
        return;
    }
    for (int i = 0; i < WM_BUTTON_COUNT; i++) {
        if (window->buttons[i] == WM_BUTTON_HOVERED) {
            set_status(window, (wm_button) i, WM_BUTTON_NORMAL);
        }
    }
}

static wm_part hovered_part(const wm_pointer_event *event)
{
    return frame_input_part_at(event->target.window, event->position);
}
