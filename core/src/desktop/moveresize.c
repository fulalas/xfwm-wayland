#include "moveresize.h"

#include "cursor.h"
#include "desktop_state.h"
#include "frame.h"
#include "keybindings.h"
#include "maximize.h"
#include "monitors.h"
#include "present.h"
#include "size_hints.h"

enum {
    KEY_MOVE_STEP = 16,
    KEY_RESIZE_STEP = 10,
    ANY_BUTTON = 0,
};

static bool begin(wm_desktop *desktop, wm_window *window, session_kind kind);
static void hold_window(wm_window *window, const move_session *session);
static bool can_begin(const wm_desktop *desktop, const wm_window *window, session_kind kind);
static void remember_cancel_state(move_session *session, const wm_window *window);
static void finish(wm_desktop *desktop);
static void finish_resize(wm_desktop *desktop, wm_window *window);
static void cancel(wm_desktop *desktop);
static void end_session(wm_desktop *desktop);
static bool release_pending(const move_session *session);
static void track_origin(wm_desktop *desktop, wm_point origin, uint32_t button);
static void handle_arrow(wm_desktop *desktop, move_direction direction);
static void resize_arrow(wm_desktop *desktop, move_direction direction);
static void move_arrow(wm_desktop *desktop, wm_point direction);
static wm_point handle_point(const wm_desktop *desktop, const wm_window *window, wm_part handle);
static int key_move_step(const wm_desktop *desktop);
static void run_motion(wm_desktop *desktop, uint32_t time_ms);

void moveresize_begin_pointer_move(wm_desktop *desktop, const session_start *start)
{
    if (begin(desktop, start->window, SESSION_MOVE)) {
        track_origin(desktop, start->origin, start->button);
    }
}

void moveresize_begin_key_move(wm_desktop *desktop, wm_window *window)
{
    if (begin(desktop, window, SESSION_MOVE)) {
        desktop->session.use_keys = true;
        moveresize_set_handle(desktop, WM_PART_NONE);
    }
}

void moveresize_begin_pointer_resize(wm_desktop *desktop, const session_start *start)
{
    if (!window_can_resize(start->window)) {
        moveresize_begin_pointer_move(desktop, start);
        return;
    }
    if (begin(desktop, start->window, SESSION_RESIZE)) {
        desktop->session.handle = start->handle;
        start->window->moving_edges = resize_moving_edges(start->handle);
        track_origin(desktop, start->origin, start->button);
        cursor_show_session(desktop);
    }
}

void moveresize_begin_key_resize(wm_desktop *desktop, wm_window *window)
{
    if (!window_can_resize(window)) {
        moveresize_begin_key_move(desktop, window);
        return;
    }
    if (begin(desktop, window, SESSION_RESIZE)) {
        desktop->session.use_keys = true;
        moveresize_set_handle(desktop, WM_PART_CORNER_BOTTOM_RIGHT);
    }
}

bool moveresize_is_active(const wm_desktop *desktop)
{
    return desktop->session.kind != SESSION_NONE;
}

bool moveresize_grabs_pointer(const wm_desktop *desktop)
{
    return moveresize_is_active(desktop) || desktop->session.waiting_release;
}

wm_dispatch moveresize_motion(wm_desktop *desktop, const wm_pointer_event *event)
{
    if (moveresize_is_active(desktop)) {
        desktop->pointer = event->position;
        run_motion(desktop, event->time_ms);
    }
    return WM_CONSUMED;
}

wm_dispatch moveresize_button(wm_desktop *desktop, const wm_button_event *event)
{
    move_session *session = &desktop->session;
    if (event->pressed) {
        return WM_CONSUMED;
    }
    if (!moveresize_is_active(desktop)) {
        session->waiting_release = session->button != ANY_BUTTON
                                   && session->button != event->button;
        return WM_CONSUMED;
    }
    bool released = session->use_keys || session->button == ANY_BUTTON
                    || session->button == event->button;
    finish(desktop);
    session->waiting_release = !released;
    return WM_CONSUMED;
}

wm_dispatch moveresize_key(wm_desktop *desktop, const wm_key_event *event)
{
    move_session *session = &desktop->session;
    const keybindings *keys = &desktop->keys;
    if (keybindings_is_key_of(keys, WM_ACTION_CANCEL, event->keysym)) {
        cancel(desktop);
        return WM_CONSUMED;
    }
    move_direction direction = keybindings_direction_of(keys, event->keysym);
    if (direction != DIRECTION_NONE) {
        handle_arrow(desktop, direction);
        return WM_CONSUMED;
    }
    if (session->use_keys && !keysym_is_modifier(event->keysym)) {
        finish(desktop);
    }
    return WM_CONSUMED;
}

void moveresize_window_gone(wm_desktop *desktop, const wm_window *window)
{
    move_session *session = &desktop->session;
    if (!moveresize_is_active(desktop) || session->window != window) {
        return;
    }
    end_session(desktop);
    session->waiting_release = release_pending(session);
}

void moveresize_set_handle(wm_desktop *desktop, wm_part handle)
{
    move_session *session = &desktop->session;
    wm_window *window = session->window;
    wm_point point = handle_point(desktop, window, handle);
    desktop->ports.commands.warp_pointer(desktop->ports.commands.self, point);
    session->handle = handle;
    window->moving_edges = resize_moving_edges(handle);
    track_origin(desktop, monitors_pointer(desktop), session->button);
    cursor_show_session(desktop);
}

void moveresize_move_pointer(wm_desktop *desktop, int dx, int dy)
{
    wm_point point = {desktop->pointer.x + dx, desktop->pointer.y + dy};
    desktop->ports.commands.warp_pointer(desktop->ports.commands.self, point);
    desktop->pointer = monitors_pointer(desktop);
    run_motion(desktop, timers_now(desktop));
}

static bool begin(wm_desktop *desktop, wm_window *window, session_kind kind)
{
    move_session *session = &desktop->session;
    if (!can_begin(desktop, window, kind)) {
        return false;
    }
    *session = (move_session) {0};
    session->kind = kind;
    session->window = window;
    remember_cancel_state(session, window);
    session->box = kind == SESSION_MOVE ? desktop->config.move.box_move
                                        : desktop->config.move.box_resize;
    hold_window(window, session);
    cursor_show_session(desktop);
    return true;
}

static void hold_window(wm_window *window, const move_session *session)
{
    if (session->kind == SESSION_MOVE) {
        maximize_save_geometry(window);
        window->moving = true;
    } else {
        window->resizing = true;
    }
    window->frozen = session->box;
    window->frozen_client = window->client;
    window_mark_dirty(window);
}

static bool can_begin(const wm_desktop *desktop, const wm_window *window, session_kind kind)
{
    if (moveresize_is_active(desktop) || window_is_fullscreen(window)) {
        return false;
    }
    uint32_t needed = kind == SESSION_MOVE ? WINDOW_HAS_MOVE : WINDOW_HAS_MOVE | WINDOW_HAS_RESIZE;
    return window_has_flag(window, needed);
}

static void remember_cancel_state(move_session *session, const wm_window *window)
{
    session->cancel_client = window->client;
    session->cancel_states = window->states;
    session->cancel_restore = window->restore_size_pos;
    session->cancel_tile = window->tile;
    session->cancel_workspace = window->workspace;
    session->is_transient = window->parent != NULL;
}

static void finish(wm_desktop *desktop)
{
    move_session *session = &desktop->session;
    wm_window *window = session->window;
    if (session->kind == SESSION_MOVE) {
        window->moving = false;
        maximize_save_geometry(window);
    } else {
        finish_resize(desktop, window);
    }
    window->frozen = false;
    window->moving_edges = 0;
    window_mark_dirty(window);
    end_session(desktop);
}

static void finish_resize(wm_desktop *desktop, wm_window *window)
{
    move_session *session = &desktop->session;
    window->resizing = false;
    if (window->client.width == session->cancel_client.width
        && window->client.height == session->cancel_client.height) {
        return;
    }
    if (window_has_state(window, WM_STATE_MAXIMIZED)) {
        maximize_remove_flag(window);
    }
    window->restore_size_pos = false;
    window->tile = TILE_NONE;
}

static void cancel(wm_desktop *desktop)
{
    if (desktop->session.kind == SESSION_MOVE) {
        move_session_cancel(desktop);
    } else {
        resize_session_cancel(desktop);
    }
    finish(desktop);
    desktop->session.waiting_release = release_pending(&desktop->session);
}

static void end_session(wm_desktop *desktop)
{
    move_session *session = &desktop->session;
    if (session->box) {
        present_hide_outline(desktop);
    }
    session->kind = SESSION_NONE;
    session->window = NULL;
    present_set_cursor(desktop, WM_CURSOR_DEFAULT);
}

static bool release_pending(const move_session *session)
{
    return !session->use_keys && session->button != ANY_BUTTON;
}

static void track_origin(wm_desktop *desktop, wm_point origin, uint32_t button)
{
    move_session *session = &desktop->session;
    wm_window *window = session->window;
    wm_rect frame = frame_rect(desktop, window);
    session->button = button;
    session->mouse_origin = origin;
    session->window_origin = (wm_point) {window->client.x, window->client.y};
    session->origin_size = (wm_size) {window->client.width, window->client.height};
    session->pointer_ratio_x = frame.width ? (origin.x - frame.x) / (double) frame.width : 0.0;
    session->pointer_ratio_y = frame.height ? (origin.y - frame.y) / (double) frame.height : 0.0;
    desktop->pointer = origin;
}

static void handle_arrow(wm_desktop *desktop, move_direction direction)
{
    if (direction == DIRECTION_NONE) {
        return;
    }
    if (desktop->session.kind == SESSION_MOVE) {
        move_arrow(desktop, direction_offset(direction));
    } else {
        resize_arrow(desktop, direction);
    }
}

static void resize_arrow(wm_desktop *desktop, move_direction direction)
{
    static const wm_part sides[] = {
        [DIRECTION_UP] = WM_PART_SIDE_TOP,
        [DIRECTION_DOWN] = WM_PART_SIDE_BOTTOM,
        [DIRECTION_LEFT] = WM_PART_SIDE_LEFT,
        [DIRECTION_RIGHT] = WM_PART_SIDE_RIGHT,
    };
    wm_point offset = direction_offset(direction);
    wm_point steps = size_hints_key_steps(desktop->session.window, KEY_RESIZE_STEP);
    wm_point step = {offset.x * steps.x, offset.y * steps.y};
    resize_session_arrow(desktop, sides[direction], step);
}

static void move_arrow(wm_desktop *desktop, wm_point direction)
{
    int step = key_move_step(desktop);
    moveresize_move_pointer(desktop, direction.x * step, direction.y * step);
}

static wm_point handle_point(const wm_desktop *desktop, const wm_window *window, wm_part handle)
{
    wm_rect frame = frame_rect(desktop, window);
    wm_extents extents = frame_extents(desktop, window);
    wm_point point = wm_rect_center(&frame);
    uint32_t edges = resize_handle_edges(handle);
    if (edges & WM_EDGE_LEFT) {
        point.x = frame.x + extents.left / 2;
    } else if (edges & WM_EDGE_RIGHT) {
        point.x = wm_rect_right(&frame) - extents.right / 2;
    }
    if (edges & WM_EDGE_TOP) {
        point.y = frame.y;
    } else if (edges & WM_EDGE_BOTTOM) {
        point.y = wm_rect_bottom(&frame) - extents.bottom / 2;
    }
    return point;
}

static int key_move_step(const wm_desktop *desktop)
{
    const wm_move_config *config = &desktop->config.move;
    int step = KEY_MOVE_STEP;
    if ((config->snap_to_border || config->snap_to_windows) && config->snap_width + 1 > step) {
        step = config->snap_width + 1;
    }
    return step;
}

static void run_motion(wm_desktop *desktop, uint32_t time_ms)
{
    if (desktop->session.kind == SESSION_MOVE) {
        move_session_motion(desktop, time_ms);
    } else if (desktop->session.kind == SESSION_RESIZE) {
        resize_session_motion(desktop);
    }
}
