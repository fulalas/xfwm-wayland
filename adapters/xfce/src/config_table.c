#include "config_table.h"

#include <glib.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "config_values.h"

enum {
    PERCENT_MAX = 100,
    HDR_BRIGHTNESS_MIN = 50,
    HDR_BRIGHTNESS_MAX = 200,
    WORKSPACE_COUNT_MIN = 1,
    TABWIN_MODE_MAX = WM_TABWIN_LIST,
};

typedef void (*config_apply_fn)(wm_config *config, size_t offset, const char *text);

typedef struct config_setting {
    const char *name;
    config_type type;
    const char *default_text;
    config_apply_fn apply;
    size_t offset;
} config_setting;

static const char *pick_text(const config_sources *sources, const char *key);
static void apply_bool(wm_config *config, size_t offset, const char *text);
static void apply_int(wm_config *config, size_t offset, const char *text);
static void apply_abs_int(wm_config *config, size_t offset, const char *text);
static void apply_percent(wm_config *config, size_t offset, const char *text);
static void apply_hdr_brightness(wm_config *config, size_t offset, const char *text);
static void apply_margin(wm_config *config, size_t offset, const char *text);
static void apply_workspace_count(wm_config *config, size_t offset, const char *text);
static void apply_tabwin_mode(wm_config *config, size_t offset, const char *text);
static void apply_button_layout(wm_config *config, size_t offset, const char *text);
static void apply_activate_action(wm_config *config, size_t offset, const char *text);
static void apply_double_click_action(wm_config *config, size_t offset, const char *text);
static void apply_placement_mode(wm_config *config, size_t offset, const char *text);
static void apply_alignment(wm_config *config, size_t offset, const char *text);
static void apply_title_shadow(wm_config *config, size_t offset, const char *text);
static void apply_easy_click(wm_config *config, size_t offset, const char *text);
static void apply_vblank_mode(wm_config *config, size_t offset, const char *text);
static void apply_render_time(wm_config *config, size_t offset, const char *text);
static void apply_double_click_overrides(wm_config *config, const setting_source *xsettings);
static void *field(wm_config *config, size_t offset);

#define AT(member) offsetof(wm_config, member)

static const config_setting config_settings[] = {
    { "activate_action", CONFIG_TEXT, "bring", apply_activate_action, AT(focus.activate_action) },
    { "borderless_maximize", CONFIG_BOOL, "true", apply_bool, AT(frame.borderless_maximize) },
    { "box_move", CONFIG_BOOL, "false", apply_bool, AT(move.box_move) },
    { "box_resize", CONFIG_BOOL, "false", apply_bool, AT(move.box_resize) },
    { "button_layout", CONFIG_TEXT, "O|SHMC", apply_button_layout, AT(frame.button_layout) },
    { "button_offset", CONFIG_INT, "0", apply_int, AT(frame.button_offset) },
    { "button_spacing", CONFIG_INT, "0", apply_int, AT(frame.button_spacing) },
    { "click_to_focus", CONFIG_BOOL, "true", apply_bool, AT(focus.click_to_focus) },
    { "cycle_apps_only", CONFIG_BOOL, "false", apply_bool, AT(cycle.apps_only) },
    { "cycle_draw_frame", CONFIG_BOOL, "true", apply_bool, AT(cycle.draw_frame) },
    { "cycle_raise", CONFIG_BOOL, "false", apply_bool, AT(cycle.raise) },
    { "cycle_hidden", CONFIG_BOOL, "true", apply_bool, AT(cycle.hidden) },
    { "cycle_minimized", CONFIG_BOOL, "false", apply_bool, AT(cycle.minimized) },
    { "cycle_minimum", CONFIG_BOOL, "true", apply_bool, AT(cycle.minimum) },
    { "cycle_preview", CONFIG_BOOL, "true", apply_bool, AT(cycle.preview) },
    { "cycle_tabwin_mode", CONFIG_INT, "0", apply_tabwin_mode, AT(cycle.tabwin_mode) },
    { "cycle_workspaces", CONFIG_BOOL, "false", apply_bool, AT(cycle.workspaces) },
    { "double_click_action", CONFIG_TEXT, "maximize", apply_double_click_action,
      AT(mouse.double_click_action) },
    { "double_click_distance", CONFIG_INT, "5", apply_abs_int, AT(mouse.double_click_distance) },
    { "double_click_time", CONFIG_INT, "250", apply_abs_int, AT(mouse.double_click_time_ms) },
    { "easy_click", CONFIG_TEXT, "Alt", apply_easy_click, AT(mouse.easy_click_modifiers) },
    { "focus_delay", CONFIG_INT, "250", apply_int, AT(focus.focus_delay_ms) },
    { "focus_hint", CONFIG_BOOL, "true", apply_bool, AT(focus.focus_hint) },
    { "focus_new", CONFIG_BOOL, "true", apply_bool, AT(focus.focus_new) },
    { "frame_border_top", CONFIG_INT, "0", apply_int, AT(frame.border_top) },
    { "frame_opacity", CONFIG_INT, "100", apply_percent, AT(compositing.frame_opacity_percent) },
    { "full_width_title", CONFIG_BOOL, "true", apply_bool, AT(frame.title.full_width) },
    { "hdr", CONFIG_BOOL, "false", apply_bool, AT(compositing.hdr) },
    { "hdr_brightness", CONFIG_INT, "100", apply_hdr_brightness,
      AT(compositing.hdr_brightness_percent) },
    { "horiz_scroll_opacity", CONFIG_BOOL, "false", apply_bool, AT(mouse.horiz_scroll_opacity) },
    { "inactive_opacity", CONFIG_INT, "100", apply_percent,
      AT(compositing.inactive_opacity_percent) },
    { "margin_bottom", CONFIG_INT, NULL, apply_margin, AT(placement.margins.bottom) },
    { "margin_left", CONFIG_INT, NULL, apply_margin, AT(placement.margins.left) },
    { "margin_right", CONFIG_INT, NULL, apply_margin, AT(placement.margins.right) },
    { "margin_top", CONFIG_INT, NULL, apply_margin, AT(placement.margins.top) },
    { "maximized_offset", CONFIG_INT, "0", apply_int, AT(frame.maximized_offset) },
    { "max_render_time", CONFIG_TEXT, "auto", apply_render_time,
      AT(compositing.max_render_time) },
    { "mousewheel_rollup", CONFIG_BOOL, "true", apply_bool, AT(mouse.mousewheel_rollup) },
    { "move_opacity", CONFIG_INT, "100", apply_percent, AT(compositing.move_opacity_percent) },
    { "placement_mode", CONFIG_TEXT, "center", apply_placement_mode, AT(placement.mode) },
    { "placement_ratio", CONFIG_INT, "20", apply_percent, AT(placement.ratio_percent) },
    { "popup_opacity", CONFIG_INT, "100", apply_percent, AT(compositing.popup_opacity_percent) },
    { "prevent_focus_stealing", CONFIG_BOOL, "false", apply_bool,
      AT(focus.prevent_focus_stealing) },
    { "raise_delay", CONFIG_INT, "250", apply_int, AT(focus.raise_delay_ms) },
    { "raise_on_click", CONFIG_BOOL, "true", apply_bool, AT(focus.raise_on_click) },
    { "raise_on_focus", CONFIG_BOOL, "false", apply_bool, AT(focus.raise_on_focus) },
    { "raise_with_any_button", CONFIG_BOOL, "true", apply_bool, AT(focus.raise_with_any_button) },
    { "repeat_urgent_blink", CONFIG_BOOL, "false", apply_bool, AT(focus.repeat_urgent_blink) },
    { "resize_opacity", CONFIG_INT, "100", apply_percent, AT(compositing.resize_opacity_percent) },
    { "scroll_workspaces", CONFIG_BOOL, "true", apply_bool, AT(mouse.scroll_workspaces) },
    { "shadow_delta_height", CONFIG_INT, "0", apply_int, AT(compositing.shadow_delta_height) },
    { "shadow_delta_width", CONFIG_INT, "0", apply_int, AT(compositing.shadow_delta_width) },
    { "shadow_delta_x", CONFIG_INT, "0", apply_int, AT(compositing.shadow_delta_x) },
    { "shadow_delta_y", CONFIG_INT, "-3", apply_int, AT(compositing.shadow_delta_y) },
    { "shadow_opacity", CONFIG_INT, "50", apply_percent, AT(compositing.shadow_opacity_percent) },
    { "show_app_icon", CONFIG_BOOL, "false", apply_bool, AT(frame.show_app_icon) },
    { "show_dock_shadow", CONFIG_BOOL, "true", apply_bool, AT(compositing.show_dock_shadow) },
    { "show_frame_shadow", CONFIG_BOOL, "true", apply_bool, AT(compositing.show_frame_shadow) },
    { "show_popup_shadow", CONFIG_BOOL, "false", apply_bool, AT(compositing.show_popup_shadow) },
    { "snap_resist", CONFIG_BOOL, "false", apply_bool, AT(move.snap_resist) },
    { "snap_to_border", CONFIG_BOOL, "true", apply_bool, AT(move.snap_to_border) },
    { "snap_to_windows", CONFIG_BOOL, "false", apply_bool, AT(move.snap_to_windows) },
    { "snap_width", CONFIG_INT, "10", apply_int, AT(move.snap_width) },
    { "suspend_compositing_fullscreen", CONFIG_BOOL, "true", apply_bool,
      AT(compositing.suspend_fullscreen) },
    { "ten_bit_color", CONFIG_BOOL, "false", apply_bool, AT(compositing.ten_bit_color) },
    { "theme", CONFIG_TEXT, "Default", NULL, 0 },
    { "tile_on_move", CONFIG_BOOL, "true", apply_bool, AT(move.tile_on_move) },
    { "title_alignment", CONFIG_TEXT, "center", apply_alignment, AT(frame.title.alignment) },
    { "title_font", CONFIG_TEXT, "Sans Bold 9", NULL, 0 },
    { "title_horizontal_offset", CONFIG_INT, "0", apply_int, AT(frame.title.horizontal_offset) },
    { "titleless_maximize", CONFIG_BOOL, "false", apply_bool, AT(frame.titleless_maximize) },
    { "title_shadow_active", CONFIG_TEXT, "false", apply_title_shadow,
      AT(frame.title.shadow_active) },
    { "title_shadow_inactive", CONFIG_TEXT, "false", apply_title_shadow,
      AT(frame.title.shadow_inactive) },
    { "title_vertical_offset_active", CONFIG_INT, "0", apply_int,
      AT(frame.title.vertical_offset_active) },
    { "title_vertical_offset_inactive", CONFIG_INT, "0", apply_int,
      AT(frame.title.vertical_offset_inactive) },
    { "toggle_workspaces", CONFIG_BOOL, "false", apply_bool, AT(workspaces.toggle) },
    { "unredirect_overlays", CONFIG_BOOL, "true", NULL, 0 },
    { "urgent_blink", CONFIG_BOOL, "false", apply_bool, AT(focus.urgent_blink) },
    { "use_compositing", CONFIG_BOOL, "true", apply_bool, AT(compositing.enabled) },
    { "use_gl_compositing", CONFIG_BOOL, "true", apply_bool, AT(compositing.use_gl) },
    { "vblank_mode", CONFIG_TEXT, "auto", apply_vblank_mode, AT(compositing.vblank_mode) },
    { "workspace_count", CONFIG_INT, "4", apply_workspace_count, AT(workspaces.count) },
    { "wrap_cycle", CONFIG_BOOL, "true", apply_bool, AT(workspaces.wrap_cycle) },
    { "wrap_layout", CONFIG_BOOL, "true", apply_bool, AT(workspaces.wrap_layout) },
    { "wrap_resistance", CONFIG_INT, "10", apply_int, AT(move.wrap_resistance) },
    { "wrap_windows", CONFIG_BOOL, "true", apply_bool, AT(move.wrap_windows) },
    { "wrap_workspaces", CONFIG_BOOL, "false", apply_bool, AT(move.wrap_workspaces) },
    { "zoom_desktop", CONFIG_BOOL, "true", apply_bool, AT(mouse.zoom_desktop) },
    { "zoom_pointer", CONFIG_BOOL, "true", apply_bool, AT(mouse.zoom_pointer) },
};

void config_apply_sources(wm_config *config, const config_sources *sources)
{
    for (size_t i = 0; i < G_N_ELEMENTS(config_settings); i++) {
        const config_setting *setting = &config_settings[i];
        const char *text = setting->apply ? pick_text(sources, setting->name) : NULL;
        if (text) {
            setting->apply(config, setting->offset, text);
        }
    }
    apply_double_click_overrides(config, &sources->xsettings);
}

void config_visit_defaults(const config_default_visitor *visitor)
{
    for (size_t i = 0; i < G_N_ELEMENTS(config_settings); i++) {
        const config_setting *setting = &config_settings[i];
        config_default entry = { setting->name, setting->type, setting->default_text };
        if (entry.text) {
            visitor->visit(visitor->self, &entry);
        }
    }
}

const char *config_default_text(const char *name)
{
    for (size_t i = 0; i < G_N_ELEMENTS(config_settings); i++) {
        if (strcmp(config_settings[i].name, name) == 0) {
            return config_settings[i].default_text;
        }
    }
    return NULL;
}

void config_set_workspace_names(wm_config *config, const char *const *names)
{
    for (size_t i = 0; names[i] && i < WM_WORKSPACE_MAX; i++) {
        char *target = config->workspaces.names[i];
        const char *valid_end;

        g_strlcpy(target, names[i], WM_WORKSPACE_NAME_MAX);
        g_utf8_validate(target, -1, &valid_end);
        target[valid_end - target] = '\0';
    }
}

static const char *pick_text(const config_sources *sources, const char *key)
{
    const char *value = setting_source_lookup(&sources->theme, key);

    if (value) {
        return value;
    }
    return setting_source_lookup(&sources->stored, key);
}

static void apply_double_click_overrides(wm_config *config, const setting_source *xsettings)
{
    const char *time_text = setting_source_lookup(xsettings, "Net/DoubleClickTime");
    const char *distance_text = setting_source_lookup(xsettings, "Net/DoubleClickDistance");

    if (time_text) {
        apply_abs_int(config, AT(mouse.double_click_time_ms), time_text);
    }
    if (distance_text) {
        apply_abs_int(config, AT(mouse.double_click_distance), distance_text);
    }
}

static void apply_bool(wm_config *config, size_t offset, const char *text)
{
    *(bool *) field(config, offset) = config_bool(text);
}

static void apply_int(wm_config *config, size_t offset, const char *text)
{
    *(int *) field(config, offset) = config_int(text);
}

static void apply_abs_int(wm_config *config, size_t offset, const char *text)
{
    *(int *) field(config, offset) = abs(config_int(text));
}

static void apply_percent(wm_config *config, size_t offset, const char *text)
{
    *(int *) field(config, offset) = CLAMP(config_int(text), 0, PERCENT_MAX);
}

static void apply_hdr_brightness(wm_config *config, size_t offset, const char *text)
{
    *(int *) field(config, offset) =
        CLAMP(config_int(text), HDR_BRIGHTNESS_MIN, HDR_BRIGHTNESS_MAX);
}

static void apply_margin(wm_config *config, size_t offset, const char *text)
{
    *(int *) field(config, offset) = MAX(config_int(text), 0);
}

static void apply_workspace_count(wm_config *config, size_t offset, const char *text)
{
    int count = CLAMP(config_int(text), WORKSPACE_COUNT_MIN, WM_WORKSPACE_MAX);

    *(int *) field(config, offset) = count;
}

static void apply_tabwin_mode(wm_config *config, size_t offset, const char *text)
{
    int mode = CLAMP(config_int(text), WM_TABWIN_GRID, TABWIN_MODE_MAX);

    *(wm_tabwin_mode *) field(config, offset) = (wm_tabwin_mode) mode;
}

static void apply_button_layout(wm_config *config, size_t offset, const char *text)
{
    g_strlcpy(field(config, offset), text, WM_BUTTON_LAYOUT_MAX);
}

static void apply_activate_action(wm_config *config, size_t offset, const char *text)
{
    *(wm_activate_action *) field(config, offset) = config_activate_action(text);
}

static void apply_double_click_action(wm_config *config, size_t offset, const char *text)
{
    *(wm_double_click_action *) field(config, offset) = config_double_click_action(text);
}

static void apply_placement_mode(wm_config *config, size_t offset, const char *text)
{
    *(wm_placement_mode *) field(config, offset) = config_placement_mode(text);
}

static void apply_alignment(wm_config *config, size_t offset, const char *text)
{
    *(wm_alignment *) field(config, offset) = config_alignment(text);
}

static void apply_title_shadow(wm_config *config, size_t offset, const char *text)
{
    *(wm_title_shadow *) field(config, offset) = config_title_shadow(text);
}

static void apply_easy_click(wm_config *config, size_t offset, const char *text)
{
    *(uint32_t *) field(config, offset) = config_easy_click(text);
}

static void apply_vblank_mode(wm_config *config, size_t offset, const char *text)
{
    *(wm_vblank_mode *) field(config, offset) = config_vblank_mode(text);
}

static void apply_render_time(wm_config *config, size_t offset, const char *text)
{
    *(wm_render_time *) field(config, offset) = config_render_time(text);
}

static void *field(wm_config *config, size_t offset)
{
    return (char *) config + offset;
}
