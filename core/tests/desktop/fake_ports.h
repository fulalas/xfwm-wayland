#ifndef FAKE_PORTS_H
#define FAKE_PORTS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "wm/desktop.h"

enum {
    FAKE_OUTPUT_MAX = 4,
    FAKE_WINDOW_MAX = 64,
    FAKE_TEXT_MAX = 128,
    FAKE_SCREEN_WIDTH = 1920,
    FAKE_SCREEN_HEIGHT = 1080,
    FAKE_SIDE = 4,
    FAKE_TITLE = 20,
    FAKE_BUTTON = 16,
    FAKE_CORNER = 6,
};

typedef struct fake_configure {
    void *window;
    wm_rect rect;
} fake_configure;

typedef struct fake_world {
    wm_ports ports;
    wm_desktop *desktop;
    wm_output_info outputs[FAKE_OUTPUT_MAX];
    size_t output_count;
    wm_point pointer;
    uint32_t now_ms;
    int window_changed_calls;
    void *stacking[FAKE_WINDOW_MAX];
    size_t stacking_count;
    int stacking_calls;
    void *focused;
    int focus_calls;
    wm_workspaces_view workspaces;
    int workspaces_calls;
    wm_cycle_view cycle;
    bool cycle_visible;
    wm_menu_view menu;
    bool menu_visible;
    wm_rect outline;
    bool outline_visible;
    wm_size_view size;
    bool size_visible;
    bool busy;
    wm_zoom_view zoom;
    int zoom_calls;
    wm_cursor_shape cursor;
    fake_configure configures[FAKE_WINDOW_MAX];
    size_t configure_count;
    int configure_calls;
    void *closed;
    void *terminate_asked;
    wm_point warp;
    int warp_calls;
    char spawned[FAKE_TEXT_MAX];
    uint32_t app_shortcut_id;
    int app_shortcut_presses;
    int app_shortcut_releases;
    int logout_calls;
    int stored_workspace_count;
    int stored_name_count;
    int stored_rows;
    int store_calls;
    bool timer_pending;
    uint32_t timer_deadline_ms;
} fake_world;

void fake_world_init(fake_world *world);
void fake_world_add_output(fake_world *world, wm_rect full, wm_rect usable);
wm_config fake_config(void);
wm_theme_metrics fake_theme(void);
wm_desktop *fake_desktop_create(fake_world *world, const wm_config *config);
void *fake_handle(int id);
wm_window_desc fake_desc(int id, wm_rect geometry);
wm_window_desc fake_desc_at(int id, wm_rect geometry);
wm_window *fake_map(wm_desktop *desktop, int id, wm_rect geometry);
wm_window *fake_map_at(wm_desktop *desktop, int id, wm_rect geometry);
wm_window *fake_map_with_hints(wm_desktop *desktop, wm_rect geometry,
                               const wm_size_hints *hints);
wm_window_desc fake_dialog_desc(int id, wm_window *parent);
wm_window *fake_map_dialog(wm_desktop *desktop, int id, wm_window *parent);
wm_rect fake_client_of(const wm_window *window);
wm_rect fake_configured(const fake_world *world, void *handle);
size_t fake_stack_index(const fake_world *world, void *handle);
void fake_advance(fake_world *world, wm_desktop *desktop, uint32_t ms);

#endif
