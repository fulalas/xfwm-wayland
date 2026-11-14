#ifndef DESKTOP_MOVERESIZE_H
#define DESKTOP_MOVERESIZE_H

#include <stdbool.h>
#include <stdint.h>

#include "window.h"

typedef enum session_kind {
    SESSION_NONE,
    SESSION_MOVE,
    SESSION_RESIZE,
} session_kind;

typedef struct move_session {
    session_kind kind;
    wm_window *window;
    bool use_keys;
    uint32_t button;
    bool waiting_release;
    wm_rect cancel_client;
    uint32_t cancel_states;
    bool cancel_restore;
    tile_mode cancel_tile;
    int cancel_workspace;
    wm_point mouse_origin;
    wm_point window_origin;
    wm_size origin_size;
    double pointer_ratio_x;
    double pointer_ratio_y;
    wm_part handle;
    bool is_transient;
    bool box;
} move_session;

typedef struct session_start {
    wm_window *window;
    wm_part handle;
    wm_point origin;
    uint32_t button;
} session_start;

void moveresize_begin_pointer_move(wm_desktop *desktop, const session_start *start);
void moveresize_begin_key_move(wm_desktop *desktop, wm_window *window);
void moveresize_begin_pointer_resize(wm_desktop *desktop, const session_start *start);
void moveresize_begin_key_resize(wm_desktop *desktop, wm_window *window);
bool moveresize_is_active(const wm_desktop *desktop);
bool moveresize_grabs_pointer(const wm_desktop *desktop);
wm_dispatch moveresize_motion(wm_desktop *desktop, const wm_pointer_event *event);
wm_dispatch moveresize_button(wm_desktop *desktop, const wm_button_event *event);
wm_dispatch moveresize_key(wm_desktop *desktop, const wm_key_event *event);
bool moveresize_key_repeats(const wm_desktop *desktop, const wm_key_event *event);
void moveresize_window_gone(wm_desktop *desktop, wm_window *window);
void moveresize_set_handle(wm_desktop *desktop, wm_part handle);
void moveresize_move_pointer(wm_desktop *desktop, int dx, int dy);

void move_session_motion(wm_desktop *desktop, uint32_t time_ms);
void move_session_cancel(wm_desktop *desktop);
void resize_session_motion(wm_desktop *desktop);
uint32_t resize_moving_edges(wm_part handle);
uint32_t resize_handle_edges(wm_part handle);
void resize_session_cancel(wm_desktop *desktop);
void resize_session_arrow(wm_desktop *desktop, wm_part side, wm_point step);

#endif
