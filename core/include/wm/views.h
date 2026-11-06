#ifndef WM_VIEWS_H
#define WM_VIEWS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "wm/config.h"
#include "wm/decor.h"
#include "wm/geometry.h"

enum {
    WM_CYCLE_ITEM_MAX = 256,
    WM_MENU_ITEM_MAX = 48,
};

typedef enum wm_layer {
    WM_LAYER_DESKTOP = 0,
    WM_LAYER_BELOW,
    WM_LAYER_NORMAL,
    WM_LAYER_ABOVE,
    WM_LAYER_DOCK,
    WM_LAYER_FULLSCREEN,
    WM_LAYER_COUNT
} wm_layer;

typedef struct wm_opacity_view {
    double content;
    double frame;
    double shadow;
} wm_opacity_view;

enum wm_allowed_action {
    WM_ALLOW_MOVE = 1 << 0,
    WM_ALLOW_RESIZE = 1 << 1,
    WM_ALLOW_MINIMIZE = 1 << 2,
    WM_ALLOW_SHADE = 1 << 3,
    WM_ALLOW_STICK = 1 << 4,
    WM_ALLOW_MAXIMIZE = 1 << 5,
    WM_ALLOW_FULLSCREEN = 1 << 6,
    WM_ALLOW_CHANGE_WORKSPACE = 1 << 7,
    WM_ALLOW_CLOSE = 1 << 8,
    WM_ALLOW_ABOVE = 1 << 9,
    WM_ALLOW_BELOW = 1 << 10,
};

typedef struct wm_window_view {
    wm_rect client;
    wm_rect shown_client;
    wm_rect restore_client;
    wm_rect frame;
    bool visible;
    bool activated;
    bool maximized;
    bool maximized_horizontally;
    bool maximized_vertically;
    bool fullscreen;
    bool minimized;
    bool shaded;
    bool sticky;
    bool above;
    bool below;
    bool urgent;
    bool moving;
    bool resizing;
    uint32_t tiled_edges;
    int workspace;
    unsigned workspace_id;
    wm_layer layer;
    bool has_shadow;
    wm_rect shadow;
    wm_opacity_view opacity;
    const wm_decor_layout *decor;
    unsigned decor_serial;
    const char *title;
    const char *app_id;
    void *parent;
    bool skip_taskbar;
    bool skip_pager;
    uint32_t allowed_actions;
} wm_window_view;

typedef struct wm_workspaces_view {
    int count;
    int current;
    int columns;
    const char *names[WM_WORKSPACE_MAX];
    unsigned ids[WM_WORKSPACE_MAX];
} wm_workspaces_view;

typedef struct wm_cycle_item {
    void *window;
    const char *title;
    bool minimized;
} wm_cycle_item;

typedef struct wm_cycle_view {
    wm_tabwin_mode mode;
    bool preview;
    int columns;
    int rows;
    int item_size;
    wm_rect monitor;
    size_t count;
    size_t selected;
    wm_cycle_item items[WM_CYCLE_ITEM_MAX];
} wm_cycle_view;

typedef enum wm_menu_label {
    WM_MENU_SEPARATOR = 0,
    WM_MENU_MAXIMIZE,
    WM_MENU_UNMAXIMIZE,
    WM_MENU_MINIMIZE,
    WM_MENU_MINIMIZE_OTHERS,
    WM_MENU_SHOW,
    WM_MENU_MOVE,
    WM_MENU_RESIZE,
    WM_MENU_ABOVE,
    WM_MENU_NORMAL,
    WM_MENU_BELOW,
    WM_MENU_SHADE,
    WM_MENU_UNSHADE,
    WM_MENU_FULLSCREEN,
    WM_MENU_UNFULLSCREEN,
    WM_MENU_STICK,
    WM_MENU_WORKSPACES,
    WM_MENU_MONITORS,
    WM_MENU_WORKSPACE_ENTRY,
    WM_MENU_MONITOR_LEFT,
    WM_MENU_MONITOR_RIGHT,
    WM_MENU_MONITOR_UP,
    WM_MENU_MONITOR_DOWN,
    WM_MENU_CLOSE,
} wm_menu_label;

typedef enum wm_menu_mark {
    WM_MENU_MARK_NONE,
    WM_MENU_MARK_CHECK_ON,
    WM_MENU_MARK_CHECK_OFF,
    WM_MENU_MARK_RADIO_ON,
    WM_MENU_MARK_RADIO_OFF,
} wm_menu_mark;

typedef struct wm_menu_item {
    wm_menu_label label;
    const char *text;
    bool sensitive;
    bool has_submenu;
    wm_menu_mark mark;
} wm_menu_item;

typedef struct wm_menu_level {
    wm_point anchor;
    size_t count;
    int selected;
    wm_menu_item items[WM_MENU_ITEM_MAX];
} wm_menu_level;

typedef struct wm_menu_view {
    wm_menu_level main;
    bool submenu_open;
    wm_menu_level submenu;
} wm_menu_view;

typedef struct wm_size_view {
    wm_rect frame;
    int columns;
    int rows;
} wm_size_view;

typedef struct wm_zoom_view {
    double level;
    wm_point center;
    bool zoom_pointer;
} wm_zoom_view;

typedef enum wm_cursor_shape {
    WM_CURSOR_DEFAULT,
    WM_CURSOR_MOVE,
    WM_CURSOR_RESIZE_TOP_LEFT,
    WM_CURSOR_RESIZE_TOP,
    WM_CURSOR_RESIZE_TOP_RIGHT,
    WM_CURSOR_RESIZE_LEFT,
    WM_CURSOR_RESIZE_RIGHT,
    WM_CURSOR_RESIZE_BOTTOM_LEFT,
    WM_CURSOR_RESIZE_BOTTOM,
    WM_CURSOR_RESIZE_BOTTOM_RIGHT,
    WM_CURSOR_BUSY,
} wm_cursor_shape;

#endif
