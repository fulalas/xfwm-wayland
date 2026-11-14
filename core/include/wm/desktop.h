#ifndef WM_DESKTOP_H
#define WM_DESKTOP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "wm/config.h"
#include "wm/decor.h"
#include "wm/geometry.h"
#include "wm/keys.h"
#include "wm/ports.h"
#include "wm/rules.h"
#include "wm/views.h"

typedef struct wm_desktop wm_desktop;
typedef struct wm_window wm_window;

typedef enum wm_window_type {
    WM_TYPE_UNSPECIFIED = 0,
    WM_TYPE_NORMAL,
    WM_TYPE_DIALOG,
    WM_TYPE_UTILITY,
    WM_TYPE_TOOLBAR,
    WM_TYPE_SPLASH,
    WM_TYPE_MENU,
    WM_TYPE_NOTIFICATION,
    WM_TYPE_DOCK,
    WM_TYPE_DESKTOP,
} wm_window_type;

enum wm_capability {
    WM_CAP_DECORATED = 1 << 0,
    WM_CAP_CLOSE = 1 << 1,
    WM_CAP_MAXIMIZE = 1 << 2,
    WM_CAP_MINIMIZE = 1 << 3,
    WM_CAP_RESIZE = 1 << 4,
    WM_CAP_MOVE = 1 << 5,
    WM_CAP_FOCUS = 1 << 6,
    WM_CAP_ALL = (1 << 7) - 1,
};

enum wm_state {
    WM_STATE_MAXIMIZED_HORZ = 1 << 0,
    WM_STATE_MAXIMIZED_VERT = 1 << 1,
    WM_STATE_MAXIMIZED = WM_STATE_MAXIMIZED_HORZ | WM_STATE_MAXIMIZED_VERT,
    WM_STATE_FULLSCREEN = 1 << 2,
    WM_STATE_MINIMIZED = 1 << 3,
    WM_STATE_SHADED = 1 << 4,
    WM_STATE_STICKY = 1 << 5,
    WM_STATE_ABOVE = 1 << 6,
    WM_STATE_BELOW = 1 << 7,
    WM_STATE_SKIP_TASKBAR = 1 << 8,
    WM_STATE_SKIP_PAGER = 1 << 9,
    WM_STATE_MODAL = 1 << 10,
    WM_STATE_URGENT = 1 << 11,
    WM_STATE_HIDE_TITLEBAR = 1 << 12,
};

typedef struct wm_state_change {
    uint32_t set;
    uint32_t unset;
} wm_state_change;

typedef struct wm_size_hints {
    wm_size min;
    wm_size max;
    wm_size base;
    wm_size step;
    wm_size min_aspect;
    wm_size max_aspect;
} wm_size_hints;

typedef struct wm_window_desc {
    void *handle;
    wm_window_type type;
    uint32_t capabilities;
    uint32_t states;
    wm_rect geometry;
    bool has_position;
    wm_point gravity_offset;
    wm_size_hints size_hints;
    wm_window *parent;
    const char *title;
    const char *app_id;
    double opacity;
    bool activation_allowed;
    bool has_icon;
    bool has_workspace;
    int workspace;
    const char *launch_id;
    bool has_user_time;
    uint32_t user_time;
    bool opacity_locked;
} wm_window_desc;

typedef enum wm_activation_source {
    WM_ACTIVATION_APPLICATION,
    WM_ACTIVATION_APPLICATION_UNTRUSTED,
    WM_ACTIVATION_PAGER,
} wm_activation_source;

typedef enum wm_target_kind {
    WM_TARGET_DESKTOP,
    WM_TARGET_CLIENT,
    WM_TARGET_FRAME,
    WM_TARGET_OTHER,
    WM_TARGET_MENU,
    WM_TARGET_SUBMENU,
    WM_TARGET_CYCLE,
} wm_target_kind;

typedef struct wm_pointer_target {
    wm_target_kind kind;
    wm_window *window;
    int item;
} wm_pointer_target;

typedef struct wm_pointer_event {
    wm_point position;
    uint32_t time_ms;
    uint32_t modifiers;
    wm_pointer_target target;
} wm_pointer_event;

enum wm_mouse_button {
    WM_MOUSE_LEFT = 1,
    WM_MOUSE_MIDDLE = 2,
    WM_MOUSE_RIGHT = 3,
    WM_MOUSE_BACK = 8,
    WM_MOUSE_FORWARD = 9,
};

typedef struct wm_button_event {
    wm_pointer_event pointer;
    uint32_t button;
    bool pressed;
} wm_button_event;

typedef enum wm_axis {
    WM_AXIS_VERTICAL,
    WM_AXIS_HORIZONTAL,
} wm_axis;

typedef struct wm_axis_event {
    wm_pointer_event pointer;
    wm_axis axis;
    int steps;
} wm_axis_event;

typedef struct wm_key_event {
    uint32_t keysym;
    uint32_t modifiers;
    uint32_t time_ms;
} wm_key_event;

typedef enum wm_dispatch {
    WM_FORWARD,
    WM_CONSUMED,
} wm_dispatch;

wm_desktop *wm_desktop_create(const wm_ports *ports, const wm_config *config);
void wm_desktop_destroy(wm_desktop *desktop);
void wm_desktop_set_appearance(wm_desktop *desktop, const wm_theme_metrics *theme,
                               const wm_config *config);
void wm_desktop_set_keymap(wm_desktop *desktop, const wm_keymap *keymap);
void wm_desktop_outputs_changed(wm_desktop *desktop);
void wm_desktop_handle_timer(wm_desktop *desktop);

void wm_desktop_switch_workspace(wm_desktop *desktop, int workspace);
void wm_desktop_add_workspace(wm_desktop *desktop);
void wm_desktop_remove_workspace(wm_desktop *desktop, int workspace);
void wm_desktop_set_workspace_count(wm_desktop *desktop, int count);
void wm_desktop_rename_workspaces(wm_desktop *desktop, const char *const *names, size_t count);
void wm_desktop_set_workspace_layout(wm_desktop *desktop, int rows, int columns);
void wm_desktop_set_window_rules(wm_desktop *desktop, const wm_window_rule *rules, size_t count);
wm_extents wm_desktop_frame_extents(const wm_desktop *desktop, uint32_t capabilities);
wm_size wm_desktop_client_bounds(const wm_desktop *desktop, uint32_t capabilities);

wm_window *wm_window_map(wm_desktop *desktop, const wm_window_desc *desc);
void wm_window_unmap(wm_desktop *desktop, wm_window *window);
wm_window_view wm_window_current_view(const wm_window *window);

enum {
    WM_LAUNCH_ID_MAX = 128,
};

void wm_launch_started(wm_desktop *desktop, const char *id);
void wm_launch_finished(wm_desktop *desktop, const char *id);
bool wm_window_launched(wm_desktop *desktop, wm_window *window, const char *id);

void wm_window_set_title(wm_desktop *desktop, wm_window *window, const char *title);
void wm_window_set_app_id(wm_desktop *desktop, wm_window *window, const char *app_id);
void wm_window_set_type(wm_desktop *desktop, wm_window *window, wm_window_type type);
void wm_window_set_capabilities(wm_desktop *desktop, wm_window *window, uint32_t capabilities);
void wm_window_set_size_hints(wm_desktop *desktop, wm_window *window, const wm_size_hints *hints);
void wm_window_set_parent(wm_desktop *desktop, wm_window *window, wm_window *parent);
void wm_window_set_opacity(wm_desktop *desktop, wm_window *window, double opacity);
void wm_window_set_opaque(wm_desktop *desktop, wm_window *window, bool opaque);
void wm_window_size_committed(wm_desktop *desktop, wm_window *window, wm_size size);
void wm_window_content_drawn(wm_desktop *desktop, wm_window *window, wm_size size);
void wm_window_demand_attention(wm_desktop *desktop, wm_window *window);
void wm_window_clear_attention(wm_desktop *desktop, wm_window *window);
void wm_window_not_responding(wm_desktop *desktop, wm_window *window);
void wm_window_icon_changed(wm_desktop *desktop, wm_window *window);

void wm_request_move(wm_desktop *desktop, wm_window *window);
void wm_request_resize(wm_desktop *desktop, wm_window *window, uint32_t edges);
void wm_window_set_fullscreen_area(wm_desktop *desktop, wm_window *window, const wm_rect *area);
void wm_window_set_fullscreen_area_once(wm_desktop *desktop, wm_window *window,
                                       const wm_rect *area);
void wm_request_window_menu(wm_desktop *desktop, wm_window *window, wm_point client_point);
void wm_request_configure(wm_desktop *desktop, wm_window *window, const wm_rect *client);
void wm_request_activate(wm_desktop *desktop, wm_window *window, wm_activation_source source);
void wm_request_close(wm_desktop *desktop, wm_window *window);
void wm_request_state(wm_desktop *desktop, wm_window *window, const wm_state_change *change);
void wm_request_send_to_monitor(wm_desktop *desktop, wm_window *window, size_t monitor);
void wm_request_keyboard_move(wm_desktop *desktop, wm_window *window);
void wm_request_keyboard_resize(wm_desktop *desktop, wm_window *window);
void wm_request_highlight(wm_desktop *desktop, wm_window *window);
void wm_request_unhighlight(wm_desktop *desktop);
void wm_request_workspace(wm_desktop *desktop, wm_window *window, int workspace);

wm_dispatch wm_pointer_motion(wm_desktop *desktop, const wm_pointer_event *event);
void wm_pointer_track(wm_desktop *desktop, wm_point position);
bool wm_pointer_repick(wm_desktop *desktop, const wm_pointer_event *event, wm_dispatch *result);
wm_dispatch wm_pointer_button(wm_desktop *desktop, const wm_button_event *event);
wm_dispatch wm_pointer_axis(wm_desktop *desktop, const wm_axis_event *event);
wm_dispatch wm_key_pressed(wm_desktop *desktop, const wm_key_event *event);
bool wm_key_repeats(const wm_desktop *desktop, const wm_key_event *event);
wm_dispatch wm_key_released(wm_desktop *desktop, const wm_key_event *event);
bool wm_key_is_bound(const wm_desktop *desktop, const wm_key_event *event);
void wm_modifiers_changed(wm_desktop *desktop, uint32_t modifiers);

#endif
