#include "click.h"
#include "cursor.h"
#include "cycle.h"
#include "desktop_state.h"
#include "focus.h"
#include "frame_buttons.h"
#include "frame_input.h"
#include "menu.h"
#include "moveresize.h"
#include "present.h"
#include "wrap.h"
#include "zoom.h"

static wm_dispatch dispatch_motion(wm_desktop *desktop, const wm_pointer_event *event);
static wm_dispatch free_motion(wm_desktop *desktop, const wm_pointer_event *event);
static void update_hover_cursor(wm_desktop *desktop, const wm_pointer_event *event);
static void follow_focus(wm_desktop *desktop, const wm_pointer_event *event);
static wm_dispatch dispatch_button(wm_desktop *desktop, const wm_button_event *event);
static wm_dispatch dispatch_axis(wm_desktop *desktop, const wm_axis_event *event);
static bool has_window_target(const wm_pointer_target *target);

wm_dispatch wm_pointer_motion(wm_desktop *desktop, const wm_pointer_event *event)
{
    desktop->pointer = event->position;
    zoom_follow_pointer(desktop, event->position);
    wm_dispatch result = dispatch_motion(desktop, event);
    present_flush(desktop);
    return result;
}

wm_dispatch wm_pointer_button(wm_desktop *desktop, const wm_button_event *event)
{
    desktop->pointer = event->pointer.position;
    wm_dispatch result = dispatch_button(desktop, event);
    present_flush(desktop);
    return result;
}

wm_dispatch wm_pointer_axis(wm_desktop *desktop, const wm_axis_event *event)
{
    desktop->pointer = event->pointer.position;
    wm_dispatch result = dispatch_axis(desktop, event);
    present_flush(desktop);
    return result;
}

static wm_dispatch dispatch_motion(wm_desktop *desktop, const wm_pointer_event *event)
{
    if (menu_is_active(desktop)) {
        return menu_motion(desktop, event);
    }
    if (cycle_is_active(desktop)) {
        return cycle_motion(desktop, event);
    }
    if (click_is_pending(desktop)) {
        return click_motion(desktop, event);
    }
    if (moveresize_grabs_pointer(desktop)) {
        return moveresize_motion(desktop, event);
    }
    if (frame_buttons_is_pressing(desktop)) {
        return frame_buttons_motion(desktop, event);
    }
    return free_motion(desktop, event);
}

static wm_dispatch free_motion(wm_desktop *desktop, const wm_pointer_event *event)
{
    update_hover_cursor(desktop, event);
    follow_focus(desktop, event);
    if (desktop->config.move.wrap_workspaces) {
        wrap_at_screen_edges(desktop, NULL, event->time_ms);
    }
    return event->target.kind == WM_TARGET_FRAME ? WM_CONSUMED : WM_FORWARD;
}

static void update_hover_cursor(wm_desktop *desktop, const wm_pointer_event *event)
{
    frame_buttons_hover(desktop, event);
    if (event->target.kind != WM_TARGET_FRAME || event->target.window == NULL) {
        present_set_cursor(desktop, WM_CURSOR_DEFAULT);
        return;
    }
    wm_window *window = event->target.window;
    cursor_show_hover(desktop, window, frame_input_part_at(window, event->position));
}

static void follow_focus(wm_desktop *desktop, const wm_pointer_event *event)
{
    wm_window *window = has_window_target(&event->target) ? event->target.window : NULL;
    if (window == desktop->focus.hovered) {
        return;
    }
    desktop->focus.hovered = window;
    if (window != NULL) {
        focus_pointer_entered(desktop, window);
    }
}

static wm_dispatch dispatch_button(wm_desktop *desktop, const wm_button_event *event)
{
    if (menu_is_active(desktop)) {
        return menu_button(desktop, event);
    }
    if (cycle_is_active(desktop)) {
        return cycle_button(desktop, event);
    }
    if (click_is_pending(desktop)) {
        return click_button(desktop, event);
    }
    if (moveresize_grabs_pointer(desktop)) {
        return moveresize_button(desktop, event);
    }
    if (frame_buttons_is_pressing(desktop)) {
        return frame_buttons_button(desktop, event);
    }
    if (!event->pressed || !has_window_target(&event->pointer.target)) {
        return event->pointer.target.kind == WM_TARGET_FRAME ? WM_CONSUMED : WM_FORWARD;
    }
    return frame_input_press(desktop, event);
}

static wm_dispatch dispatch_axis(wm_desktop *desktop, const wm_axis_event *event)
{
    if (menu_is_active(desktop)) {
        return WM_CONSUMED;
    }
    if (cycle_is_active(desktop)) {
        return cycle_axis(desktop, event);
    }
    if (click_is_pending(desktop) || moveresize_grabs_pointer(desktop)
        || frame_buttons_is_pressing(desktop)) {
        return WM_CONSUMED;
    }
    return frame_input_axis(desktop, event);
}

static bool has_window_target(const wm_pointer_target *target)
{
    return (target->kind == WM_TARGET_CLIENT || target->kind == WM_TARGET_FRAME)
           && target->window != NULL;
}
