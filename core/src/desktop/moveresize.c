#include "moveresize.h"

#include "cursor.h"
#include "desktop_state.h"
#include "frame.h"
#include "keybindings.h"
#include "maximize.h"
#include "present.h"

enum {
    KEY_MOVE_STEP = 16,
    KEY_RESIZE_STEP = 10,
    ANY_BUTTON = 0,
};

static bool begin(wm_desktop *desktop, wm_window *window, session_kind kind);
static bool can_begin(const wm_desktop *desktop, const wm_window *window, session_kind kind);
static void remember_cancel_state(move_session *session, const wm_window *window);
static void finish(wm_desktop *desktop);
static void finish_resize(wm_desktop *desktop, wm_window *window);
static void cancel(wm_desktop *desktop);
static void track_origin(wm_desktop *desktop, wm_point origin, uint32_t button);
static void handle_arrow(wm_desktop *desktop, uint32_t keysym);
static void move_arrow(wm_desktop *desktop, wm_point direction);
static wm_point handle_point(const wm_desktop *desktop, const wm_window *window, wm_part handle);
static uint32_t handle_edges(wm_part handle);
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
    if (keybindings_is_key_of(keys, WM_ACTION_LEFT, event->keysym)
        || keybindings_is_key_of(keys, WM_ACTION_RIGHT, event->keysym)
        || keybindings_is_key_of(keys, WM_ACTION_UP, event->keysym)
        || keybindings_is_key_of(keys, WM_ACTION_DOWN, event->keysym)) {
        handle_arrow(desktop, event->keysym);
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
    if (session->window != window) {
        return;
    }
    if (session->box) {
        present_hide_outline(desktop);
    }
    session->kind = SESSION_NONE;
    session->window = NULL;
    session->waiting_release = session->button != ANY_BUTTON && !session->use_keys;
    present_set_cursor(desktop, WM_CURSOR_DEFAULT);
}

void moveresize_set_handle(wm_desktop *desktop, wm_part handle)
{
    move_session *session = &desktop->session;
    wm_window *window = session->window;
    wm_point point = handle_point(desktop, window, handle);
    desktop->pointer = point;
    desktop->ports.commands.warp_pointer(desktop->ports.commands.self, point);
    session->handle = handle;
    window->moving_edges = resize_moving_edges(handle);
    track_origin(desktop, point, session->button);
    cursor_show_session(desktop);
}

void moveresize_move_pointer(wm_desktop *desktop, int dx, int dy)
{
    desktop->pointer.x += dx;
    desktop->pointer.y += dy;
    desktop->ports.commands.warp_pointer(desktop->ports.commands.self, desktop->pointer);
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
    if (kind == SESSION_MOVE) {
        maximize_save_geometry(window);
        window->moving = true;
    } else {
        window->resizing = true;
    }
    window->frozen = session->box;
    window->frozen_client = window->client;
    window_mark_dirty(window);
    cursor_show_session(desktop);
    return true;
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
    window_mark_dirty(window);
    if (session->box) {
        present_hide_outline(desktop);
    }
    session->kind = SESSION_NONE;
    present_set_cursor(desktop, WM_CURSOR_DEFAULT);
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
    bool use_keys = desktop->session.use_keys;
    finish(desktop);
    desktop->session.waiting_release = !use_keys && desktop->session.button != ANY_BUTTON;
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

static void handle_arrow(wm_desktop *desktop, uint32_t keysym)
{
    const keybindings *keys = &desktop->keys;
    wm_point direction = {0, 0};
    wm_part side = WM_PART_SIDE_LEFT;
    if (keybindings_is_key_of(keys, WM_ACTION_LEFT, keysym)) {
        direction.x = -1;
    } else if (keybindings_is_key_of(keys, WM_ACTION_RIGHT, keysym)) {
        direction.x = 1;
        side = WM_PART_SIDE_RIGHT;
    } else if (keybindings_is_key_of(keys, WM_ACTION_UP, keysym)) {
        direction.y = -1;
        side = WM_PART_SIDE_TOP;
    } else {
        direction.y = 1;
        side = WM_PART_SIDE_BOTTOM;
    }
    if (desktop->session.kind == SESSION_MOVE) {
        move_arrow(desktop, direction);
    } else {
        wm_point step = {direction.x * KEY_RESIZE_STEP, direction.y * KEY_RESIZE_STEP};
        resize_session_arrow(desktop, side, step);
    }
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
    uint32_t edges = handle_edges(handle);
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

static uint32_t handle_edges(wm_part handle)
{
    switch (handle) {
    case WM_PART_CORNER_BOTTOM_LEFT:
        return WM_EDGE_BOTTOM | WM_EDGE_LEFT;
    case WM_PART_CORNER_BOTTOM_RIGHT:
        return WM_EDGE_BOTTOM | WM_EDGE_RIGHT;
    case WM_PART_CORNER_TOP_LEFT:
        return WM_EDGE_TOP | WM_EDGE_LEFT;
    case WM_PART_CORNER_TOP_RIGHT:
        return WM_EDGE_TOP | WM_EDGE_RIGHT;
    case WM_PART_SIDE_LEFT:
        return WM_EDGE_LEFT;
    case WM_PART_SIDE_RIGHT:
        return WM_EDGE_RIGHT;
    case WM_PART_SIDE_TOP:
        return WM_EDGE_TOP;
    case WM_PART_SIDE_BOTTOM:
        return WM_EDGE_BOTTOM;
    default:
        return WM_EDGE_NONE;
    }
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
