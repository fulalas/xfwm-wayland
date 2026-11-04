#include "wm/config.h"

#include <string.h>

#include "wm/keys.h"

enum {
    DEFAULT_BUTTON_OFFSET = 0,
    DEFAULT_BUTTON_SPACING = 0,
    DEFAULT_MAXIMIZED_OFFSET = 0,
    DEFAULT_BORDER_TOP = 0,
    DEFAULT_TITLE_OFFSET = 0,
    DEFAULT_FOCUS_DELAY_MS = 250,
    DEFAULT_RAISE_DELAY_MS = 250,
    DEFAULT_DOUBLE_CLICK_TIME_MS = 250,
    DEFAULT_DOUBLE_CLICK_DISTANCE = 5,
    DEFAULT_SNAP_WIDTH = 10,
    DEFAULT_WRAP_RESISTANCE = 10,
    DEFAULT_PLACEMENT_RATIO_PERCENT = 20,
    DEFAULT_WORKSPACE_COUNT = 4,
    DEFAULT_OPAQUE_PERCENT = 100,
    DEFAULT_SHADOW_OPACITY_PERCENT = 50,
    DEFAULT_SHADOW_DELTA_X = 0,
    DEFAULT_SHADOW_DELTA_Y = -3,
    DEFAULT_SHADOW_DELTA_WIDTH = 0,
    DEFAULT_SHADOW_DELTA_HEIGHT = 0,
};

static const char default_button_layout[] = "O|SHMC";

static wm_frame_config frame_defaults(void);
static wm_title_config title_defaults(void);
static wm_focus_config focus_defaults(void);
static wm_mouse_config mouse_defaults(void);
static wm_move_config move_defaults(void);
static wm_placement_config placement_defaults(void);
static wm_cycle_config cycle_defaults(void);
static wm_workspace_config workspace_defaults(void);
static wm_compositing_config compositing_defaults(void);

wm_config wm_config_defaults(void)
{
    return (wm_config) {
        .frame = frame_defaults(),
        .focus = focus_defaults(),
        .mouse = mouse_defaults(),
        .move = move_defaults(),
        .placement = placement_defaults(),
        .cycle = cycle_defaults(),
        .workspaces = workspace_defaults(),
        .compositing = compositing_defaults(),
    };
}

static wm_frame_config frame_defaults(void)
{
    wm_frame_config frame = {
        .button_offset = DEFAULT_BUTTON_OFFSET,
        .button_spacing = DEFAULT_BUTTON_SPACING,
        .maximized_offset = DEFAULT_MAXIMIZED_OFFSET,
        .border_top = DEFAULT_BORDER_TOP,
        .borderless_maximize = true,
        .titleless_maximize = false,
        .show_app_icon = false,
        .title = title_defaults(),
    };
    memcpy(frame.button_layout, default_button_layout, sizeof default_button_layout);
    return frame;
}

static wm_title_config title_defaults(void)
{
    return (wm_title_config) {
        .alignment = WM_ALIGN_CENTER,
        .full_width = true,
        .horizontal_offset = DEFAULT_TITLE_OFFSET,
        .vertical_offset_active = DEFAULT_TITLE_OFFSET,
        .vertical_offset_inactive = DEFAULT_TITLE_OFFSET,
        .shadow_active = WM_TITLE_SHADOW_NONE,
        .shadow_inactive = WM_TITLE_SHADOW_NONE,
    };
}

static wm_focus_config focus_defaults(void)
{
    return (wm_focus_config) {
        .click_to_focus = true,
        .focus_delay_ms = DEFAULT_FOCUS_DELAY_MS,
        .focus_new = true,
        .focus_hint = true,
        .prevent_focus_stealing = false,
        .raise_on_focus = false,
        .raise_delay_ms = DEFAULT_RAISE_DELAY_MS,
        .raise_on_click = true,
        .raise_with_any_button = true,
        .activate_action = WM_ACTIVATE_BRING,
        .urgent_blink = false,
        .repeat_urgent_blink = false,
    };
}

static wm_mouse_config mouse_defaults(void)
{
    return (wm_mouse_config) {
        .easy_click_modifiers = WM_MOD_ALT,
        .double_click_action = WM_DOUBLE_CLICK_MAXIMIZE,
        .double_click_time_ms = DEFAULT_DOUBLE_CLICK_TIME_MS,
        .double_click_distance = DEFAULT_DOUBLE_CLICK_DISTANCE,
        .mousewheel_rollup = true,
        .horiz_scroll_opacity = false,
        .scroll_workspaces = true,
        .zoom_desktop = true,
        .zoom_pointer = true,
    };
}

static wm_move_config move_defaults(void)
{
    return (wm_move_config) {
        .box_move = false,
        .box_resize = false,
        .snap_to_border = true,
        .snap_to_windows = false,
        .snap_resist = false,
        .snap_width = DEFAULT_SNAP_WIDTH,
        .tile_on_move = true,
        .wrap_windows = true,
        .wrap_workspaces = false,
        .wrap_resistance = DEFAULT_WRAP_RESISTANCE,
    };
}

static wm_placement_config placement_defaults(void)
{
    return (wm_placement_config) {
        .mode = WM_PLACE_CENTER,
        .ratio_percent = DEFAULT_PLACEMENT_RATIO_PERCENT,
        .margins = {0, 0, 0, 0},
    };
}

static wm_cycle_config cycle_defaults(void)
{
    return (wm_cycle_config) {
        .apps_only = false,
        .draw_frame = true,
        .raise = false,
        .hidden = true,
        .minimum = true,
        .minimized = false,
        .preview = true,
        .workspaces = false,
        .tabwin_mode = WM_TABWIN_GRID,
    };
}

static wm_workspace_config workspace_defaults(void)
{
    return (wm_workspace_config) {
        .count = DEFAULT_WORKSPACE_COUNT,
        .toggle = false,
        .wrap_cycle = true,
        .wrap_layout = true,
    };
}

static wm_compositing_config compositing_defaults(void)
{
    return (wm_compositing_config) {
        .enabled = true,
        .use_gl = true,
        .frame_opacity_percent = DEFAULT_OPAQUE_PERCENT,
        .inactive_opacity_percent = DEFAULT_OPAQUE_PERCENT,
        .move_opacity_percent = DEFAULT_OPAQUE_PERCENT,
        .resize_opacity_percent = DEFAULT_OPAQUE_PERCENT,
        .popup_opacity_percent = DEFAULT_OPAQUE_PERCENT,
        .shadow_opacity_percent = DEFAULT_SHADOW_OPACITY_PERCENT,
        .shadow_delta_x = DEFAULT_SHADOW_DELTA_X,
        .shadow_delta_y = DEFAULT_SHADOW_DELTA_Y,
        .shadow_delta_width = DEFAULT_SHADOW_DELTA_WIDTH,
        .shadow_delta_height = DEFAULT_SHADOW_DELTA_HEIGHT,
        .show_frame_shadow = true,
        .show_popup_shadow = false,
        .show_dock_shadow = true,
        .suspend_fullscreen = true,
        .unredirect_overlays = true,
        .vblank_mode = WM_VBLANK_AUTO,
    };
}
