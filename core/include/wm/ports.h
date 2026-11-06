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
    void (*zoom_changed)(void *self, const wm_zoom_view *view);
    void (*cursor_changed)(void *self, wm_cursor_shape shape);
} wm_presenter;

typedef struct wm_commands {
    void *self;
    void (*configure)(void *self, void *window, const wm_rect *client);
    void (*close)(void *self, void *window);
    void (*ask_terminate)(void *self, void *window);
    void (*warp_pointer)(void *self, wm_point point);
    void (*spawn)(void *self, const char *command);
    void (*app_shortcut)(void *self, uint32_t id, bool pressed, uint32_t time_ms);
    void (*logout)(void *self);
    void (*store_workspace_count)(void *self, int count);
    void (*schedule_timer)(void *self, uint32_t delay_ms);
} wm_commands;

typedef struct wm_environment {
    void *self;
    size_t (*output_count)(void *self);
    wm_output_info (*output_at_index)(void *self, size_t index);
    wm_point (*pointer_position)(void *self);
    uint32_t (*now_ms)(void *self);
    wm_size (*measure_title)(void *self, const char *text);
} wm_environment;

typedef struct wm_ports {
    wm_presenter presenter;
    wm_commands commands;
    wm_environment environment;
} wm_ports;

#endif
