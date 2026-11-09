#ifndef DESKTOP_WINDOW_H
#define DESKTOP_WINDOW_H

#include <stdbool.h>
#include <stdint.h>

#include "wm/desktop.h"

typedef enum tile_mode {
    TILE_NONE = 0,
    TILE_LEFT,
    TILE_RIGHT,
    TILE_DOWN,
    TILE_UP,
    TILE_DOWN_LEFT,
    TILE_DOWN_RIGHT,
    TILE_UP_LEFT,
    TILE_UP_RIGHT,
} tile_mode;

enum window_flag {
    WINDOW_HAS_BORDER = 1 << 0,
    WINDOW_HAS_MENU = 1 << 1,
    WINDOW_HAS_MAXIMIZE = 1 << 2,
    WINDOW_HAS_CLOSE = 1 << 3,
    WINDOW_HAS_HIDE = 1 << 4,
    WINDOW_HAS_MOVE = 1 << 5,
    WINDOW_HAS_RESIZE = 1 << 6,
    WINDOW_HAS_STICK = 1 << 7,
    WINDOW_HAS_FOCUS = 1 << 8,
    WINDOW_IS_RESIZABLE = 1 << 9,
};

struct wm_window {
    void *handle;
    wm_window_type requested_type;
    wm_window_type type;
    uint32_t capabilities;
    uint32_t states;
    uint32_t flags;
    unsigned long serial;
    uint32_t mapped_ms;
    wm_rect client;
    wm_rect saved;
    wm_rect pre_fullscreen;
    wm_rect fullscreen_area;
    wm_rect configured;
    wm_rect lost_output;
    bool has_lost_output;
    bool configure_pending;
    bool frozen;
    wm_rect frozen_client;
    wm_size drawn;
    bool follow_drawn;
    uint32_t follow_edges;
    uint32_t moving_edges;
    wm_size_hints hints;
    wm_window *parent;
    char *title;
    char *app_id;
    uint32_t opacity;
    int workspace;
    wm_layer layer;
    tile_mode tile;
    bool restore_size_pos;
    bool visible;
    bool was_shown;
    bool workspace_focus;
    bool seen_active;
    int blink_iterations;
    bool moving;
    bool resizing;
    bool has_icon;
    bool activation_allowed;
    bool skip_initial_focus;
    bool opacity_locked;
    bool opaque;
    bool has_position;
    bool close_requested;
    bool title_measured;
    wm_size title_size;
    wm_button_status buttons[WM_BUTTON_COUNT];
    bool dirty;
    bool decor_valid;
    wm_decor_window decor_inputs;
    wm_decor_layout layout;
    unsigned decor_serial;
    wm_window_view view;
};

wm_window *window_create(const wm_window_desc *desc, unsigned long serial);
void window_destroy(wm_window *window);
void window_set_title(wm_window *window, const char *title);
void window_set_app_id(wm_window *window, const char *app_id);
void window_apply_type(wm_window *window);
void window_update_flags(wm_window *window);
uint32_t window_opacity_from_double(double opacity);

bool window_has_state(const wm_window *window, uint32_t states);
bool window_has_flag(const wm_window *window, uint32_t flags);
bool window_is_maximized(const wm_window *window);
wm_rect window_shown_client(const wm_window *window);
bool window_is_minimized(const wm_window *window);
bool window_is_fullscreen(const wm_window *window);
bool window_is_shaded(const wm_window *window);
bool window_is_sticky(const wm_window *window);
bool window_is_regular_focusable(const wm_window *window);
bool window_is_dont_focus(const wm_window *window);
bool window_is_dont_place(const wm_window *window);
bool window_is_dialog(const wm_window *window);
bool window_can_hide(const wm_window *window);
bool window_can_maximize(const wm_window *window);
bool window_allows_maximize(const wm_window *window);
bool window_can_fill(const wm_window *window);
bool window_can_tile(const wm_window *window);
bool window_can_resize(const wm_window *window);
bool window_is_constrained(const wm_window *window);
void window_mark_dirty(wm_window *window);
void window_placed(wm_window *window);

#endif
