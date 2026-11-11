#ifndef WM_PORTS_H
#define WM_PORTS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "wm/geometry.h"
#include "wm/views.h"

typedef struct wm_output_info {
    wm_rect full;
    wm_rect usable;
    bool primary;
    const char *name;
} wm_output_info;

typedef struct wm_presenter {
    void *self;
    void (*window_changed)(void *self, void *window, const wm_window_view *view);
    void (*stacking_changed)(void *self, void *const *windows_bottom_to_top, size_t count);
    void (*focus_changed)(void *self, void *window);
    void (*workspaces_changed)(void *self, const wm_workspaces_view *view);
    void (*cycle_shown)(void *self, const wm_cycle_view *view);
    void (*cycle_hidden)(void *self);
    void (*menu_shown)(void *self, const wm_menu_view *view);
    void (*menu_hidden)(void *self);
    void (*outline_shown)(void *self, const wm_rect *rect);
    void (*outline_hidden)(void *self);
    void (*size_shown)(void *self, const wm_size_view *view);
    void (*size_hidden)(void *self);
    void (*zoom_changed)(void *self, const wm_zoom_view *view);
    void (*cursor_changed)(void *self, wm_cursor_shape shape);
    void (*busy_changed)(void *self, bool busy);
} wm_presenter;

typedef enum wm_shortcut_state {
    WM_SHORTCUT_PRESSED,
    WM_SHORTCUT_RELEASED,
} wm_shortcut_state;

typedef struct wm_app_shortcut_event {
    uint32_t id;
    wm_shortcut_state state;
    uint32_t time_ms;
} wm_app_shortcut_event;

typedef struct wm_commands {
    void *self;
    void (*configure)(void *self, void *window, const wm_rect *client);
    void (*close)(void *self, void *window);
    void (*ask_terminate)(void *self, void *window);
    void (*warp_pointer)(void *self, wm_point point);
    void (*spawn)(void *self, const char *command);
    void (*app_shortcut)(void *self, const wm_app_shortcut_event *event);
    void (*logout)(void *self);
    void (*store_workspace_count)(void *self, int count);
    void (*store_workspace_names)(void *self, const char *const *names, size_t count);
    void (*store_workspace_rows)(void *self, int rows);
    void (*schedule_timer)(void *self, uint32_t delay_ms);
} wm_commands;

typedef struct wm_environment {
    void *self;
    size_t (*output_count)(void *self);
    wm_output_info (*output_at_index)(void *self, size_t index);
    wm_point (*pointer_position)(void *self);
    uint32_t (*now_ms)(void *self);
    wm_size (*measure_title)(void *self, const char *text);
    bool (*menu_item_matches_key)(void *self, const wm_menu_entry *entry, uint32_t keysym);
} wm_environment;

typedef struct wm_ports {
    wm_presenter presenter;
    wm_commands commands;
    wm_environment environment;
} wm_ports;

#endif
