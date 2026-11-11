#ifndef WM_CONFIG_H
#define WM_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

enum {
    WM_BUTTON_LAYOUT_MAX = 16,
    WM_WORKSPACE_NAME_MAX = 64,
    WM_WORKSPACE_MAX = 32,
};

typedef enum wm_title_shadow {
    WM_TITLE_SHADOW_NONE,
    WM_TITLE_SHADOW_UNDER,
    WM_TITLE_SHADOW_FRAME,
} wm_title_shadow;

typedef enum wm_alignment {
    WM_ALIGN_LEFT,
    WM_ALIGN_RIGHT,
    WM_ALIGN_CENTER,
} wm_alignment;

typedef enum wm_activate_action {
    WM_ACTIVATE_NONE,
    WM_ACTIVATE_BRING,
    WM_ACTIVATE_SWITCH,
} wm_activate_action;

typedef enum wm_double_click_action {
    WM_DOUBLE_CLICK_NONE,
    WM_DOUBLE_CLICK_MAXIMIZE,
    WM_DOUBLE_CLICK_SHADE,
    WM_DOUBLE_CLICK_FILL,
    WM_DOUBLE_CLICK_ABOVE,
    WM_DOUBLE_CLICK_HIDE,
} wm_double_click_action;

typedef enum wm_placement_mode {
    WM_PLACE_MOUSE,
    WM_PLACE_CENTER,
} wm_placement_mode;

typedef enum wm_vblank_mode {
    WM_VBLANK_AUTO,
    WM_VBLANK_OFF,
    WM_VBLANK_ADAPTIVE,
} wm_vblank_mode;

typedef enum wm_render_time_mode {
    WM_RENDER_TIME_OFF,
    WM_RENDER_TIME_FIXED,
    WM_RENDER_TIME_AUTO,
} wm_render_time_mode;

typedef struct wm_render_time {
    wm_render_time_mode mode;
    int ms;
} wm_render_time;

typedef enum wm_tabwin_mode {
    WM_TABWIN_GRID = 0,
    WM_TABWIN_LIST = 1,
} wm_tabwin_mode;

typedef struct wm_margins {
    int top;
    int bottom;
    int left;
    int right;
} wm_margins;

typedef struct wm_title_config {
    wm_alignment alignment;
    bool full_width;
    int horizontal_offset;
    int vertical_offset_active;
    int vertical_offset_inactive;
    wm_title_shadow shadow_active;
    wm_title_shadow shadow_inactive;
} wm_title_config;

typedef struct wm_frame_config {
    char button_layout[WM_BUTTON_LAYOUT_MAX];
    int button_offset;
    int button_spacing;
    int maximized_offset;
    int border_top;
    bool borderless_maximize;
    bool titleless_maximize;
    bool show_app_icon;
    wm_title_config title;
} wm_frame_config;

typedef struct wm_focus_config {
    bool click_to_focus;
    int focus_delay_ms;
    bool focus_new;
    bool focus_hint;
    bool prevent_focus_stealing;
    bool raise_on_focus;
    int raise_delay_ms;
    bool raise_on_click;
    bool raise_with_any_button;
    wm_activate_action activate_action;
    bool urgent_blink;
    bool repeat_urgent_blink;
} wm_focus_config;

typedef struct wm_mouse_config {
    uint32_t easy_click_modifiers;
    wm_double_click_action double_click_action;
    int double_click_time_ms;
    int double_click_distance;
    bool mousewheel_rollup;
    bool horiz_scroll_opacity;
    bool scroll_workspaces;
    bool zoom_desktop;
    bool zoom_pointer;
} wm_mouse_config;

typedef struct wm_move_config {
    bool box_move;
    bool box_resize;
    bool snap_to_border;
    bool snap_to_windows;
    bool snap_resist;
    int snap_width;
    bool tile_on_move;
    bool wrap_windows;
    bool wrap_workspaces;
    int wrap_resistance;
} wm_move_config;

typedef struct wm_placement_config {
    wm_placement_mode mode;
    int ratio_percent;
    wm_margins margins;
} wm_placement_config;

typedef struct wm_cycle_config {
    bool apps_only;
    bool draw_frame;
    bool raise;
    bool hidden;
    bool minimum;
    bool minimized;
    bool preview;
    bool workspaces;
    wm_tabwin_mode tabwin_mode;
} wm_cycle_config;

typedef struct wm_workspace_config {
    int count;
    int rows;
    bool toggle;
    bool wrap_cycle;
    bool wrap_layout;
    char names[WM_WORKSPACE_MAX][WM_WORKSPACE_NAME_MAX];
} wm_workspace_config;

typedef struct wm_compositing_config {
    bool enabled;
    bool use_gl;
    int frame_opacity_percent;
    int inactive_opacity_percent;
    int move_opacity_percent;
    int resize_opacity_percent;
    int popup_opacity_percent;
    int shadow_opacity_percent;
    int shadow_delta_x;
    int shadow_delta_y;
    int shadow_delta_width;
    int shadow_delta_height;
    bool show_frame_shadow;
    bool show_popup_shadow;
    bool show_dock_shadow;
    bool suspend_fullscreen;
    wm_vblank_mode vblank_mode;
    bool ten_bit_color;
    bool hdr;
    int hdr_brightness_percent;
    wm_render_time max_render_time;
    bool xwayland_zero_scaling;
} wm_compositing_config;

typedef struct wm_config {
    wm_frame_config frame;
    wm_focus_config focus;
    wm_mouse_config mouse;
    wm_move_config move;
    wm_placement_config placement;
    wm_cycle_config cycle;
    wm_workspace_config workspaces;
    wm_compositing_config compositing;
} wm_config;

wm_config wm_config_defaults(void);

#endif
