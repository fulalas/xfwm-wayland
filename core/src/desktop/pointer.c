#include "actions.h"
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

typedef struct pointer_grab {
    wm_dispatch (*motion)(wm_desktop *desktop, const wm_pointer_event *event);
    wm_dispatch (*button)(wm_desktop *desktop, const wm_button_event *event);
    wm_dispatch (*axis)(wm_desktop *desktop, const wm_axis_event *event);
} pointer_grab;

static const pointer_grab menu_grab = {menu_motion, menu_button, NULL};
static const pointer_grab cycle_grab = {cycle_motion, cycle_button, cycle_axis};
static const pointer_grab click_grab = {click_motion, click_button, NULL};
static const pointer_grab moveresize_grab = {moveresize_motion, moveresize_button, NULL};
static const pointer_grab frame_button_grab = {frame_buttons_motion, frame_buttons_button, NULL};

static const pointer_grab *active_grab(const wm_desktop *desktop);
static wm_dispatch dispatch_motion(wm_desktop *desktop, const wm_pointer_event *event);
static wm_dispatch free_motion(wm_desktop *desktop, const wm_pointer_event *event);
static wm_dispatch hover(wm_desktop *desktop, const wm_pointer_event *event);
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

bool wm_pointer_repick(wm_desktop *desktop, const wm_pointer_event *event, wm_dispatch *result)
{
    if (active_grab(desktop) != NULL) {
        return false;
    }
    desktop->pointer = event->position;
    *result = hover(desktop, event);
    present_flush(desktop);
    return true;
}

wm_dispatch wm_pointer_button(wm_desktop *desktop, const wm_button_event *event)
{
    desktop->pointer = event->pointer.position;
    if (event->pressed) {
        focus_note_user_input(desktop, event->pointer.time_ms);
        actions_cancel_tap(desktop);
    }
    wm_dispatch result = dispatch_button(desktop, event);
    present_flush(desktop);
    return result;
}

wm_dispatch wm_pointer_axis(wm_desktop *desktop, const wm_axis_event *event)
{
    desktop->pointer = event->pointer.position;
    actions_cancel_tap(desktop);
    wm_dispatch result = dispatch_axis(desktop, event);
    present_flush(desktop);
    return result;
}

static const pointer_grab *active_grab(const wm_desktop *desktop)
{
    if (menu_is_active(desktop)) {
        return &menu_grab;
    }
    if (cycle_is_active(desktop)) {
        return &cycle_grab;
    }
    if (click_is_pending(desktop)) {
        return &click_grab;
    }
    if (moveresize_grabs_pointer(desktop)) {
        return &moveresize_grab;
    }
    return frame_buttons_is_pressing(desktop) ? &frame_button_grab : NULL;
}

static wm_dispatch dispatch_motion(wm_desktop *desktop, const wm_pointer_event *event)
{
    const pointer_grab *grab = active_grab(desktop);
    return grab != NULL ? grab->motion(desktop, event) : free_motion(desktop, event);
}

static wm_dispatch free_motion(wm_desktop *desktop, const wm_pointer_event *event)
{
    wm_dispatch result = hover(desktop, event);
    follow_focus(desktop, event);
    if (desktop->config.move.wrap_workspaces) {
        wrap_at_screen_edges(desktop, NULL, event->time_ms);
    }
    return result;
}

static wm_dispatch hover(wm_desktop *desktop, const wm_pointer_event *event)
{
    update_hover_cursor(desktop, event);
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
    const pointer_grab *grab = active_grab(desktop);
    if (grab != NULL) {
        return grab->button(desktop, event);
    }
    if (event->pressed && event->button == WM_MOUSE_LEFT
        && event->pointer.target.kind == WM_TARGET_DESKTOP) {
        focus_desktop_clicked(desktop);
    }
    if (!event->pressed || !has_window_target(&event->pointer.target)) {
        return event->pointer.target.kind == WM_TARGET_FRAME ? WM_CONSUMED : WM_FORWARD;
    }
    return frame_input_press(desktop, event);
}

static wm_dispatch dispatch_axis(wm_desktop *desktop, const wm_axis_event *event)
{
    const pointer_grab *grab = active_grab(desktop);
    if (grab == NULL) {
        return frame_input_axis(desktop, event);
    }
    return grab->axis != NULL ? grab->axis(desktop, event) : WM_CONSUMED;
}

static bool has_window_target(const wm_pointer_target *target)
{
    return (target->kind == WM_TARGET_CLIENT || target->kind == WM_TARGET_FRAME)
           && target->window != NULL;
}
