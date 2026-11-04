#include "effects.h"

#include "desktop_state.h"
#include "frame.h"
#include "wm/shadow.h"

enum {
    PERCENT = 100,
};

static const uint32_t OPACITY_OPAQUE = 0xffffffffu;
static const uint32_t OPACITY_SET_STEP = 0x16000000u;
static const uint32_t OPACITY_SET_MIN = 0x40000000u;

static double applied_opacity(const wm_desktop *desktop, const wm_window *window);
static double percent_factor(int percent);
static bool is_inactive(const wm_desktop *desktop, const wm_window *window);
static bool uses_move_opacity(const wm_desktop *desktop, const wm_window *window);
static bool uses_resize_opacity(const wm_desktop *desktop, const wm_window *window);

wm_opacity_view effects_opacity(const wm_desktop *desktop, const wm_window *window)
{
    if (!effects_enabled(desktop)) {
        return (wm_opacity_view) {1.0, 1.0, 1.0};
    }
    const wm_compositing_config *config = &desktop->config.compositing;
    double content = applied_opacity(desktop, window);
    double frame = content * percent_factor(config->frame_opacity_percent);
    double shadow = percent_factor(config->frame_opacity_percent)
                    * percent_factor(config->shadow_opacity_percent) * content;
    return (wm_opacity_view) {content, frame, shadow};
}

bool effects_has_shadow(const wm_desktop *desktop, const wm_window *window)
{
    const wm_compositing_config *config = &desktop->config.compositing;
    if (!effects_enabled(desktop) || window_is_fullscreen(window)) {
        return false;
    }
    if (window->type == WM_TYPE_DOCK) {
        return config->show_dock_shadow;
    }
    return config->show_frame_shadow && !window_is_maximized(window)
           && frame_is_shown(desktop, window);
}

wm_rect effects_shadow_rect(const wm_desktop *desktop, const wm_window *window)
{
    if (!effects_has_shadow(desktop, window)) {
        return (wm_rect) {0, 0, 0, 0};
    }
    wm_rect frame = frame_rect(desktop, window);
    return wm_shadow_rect(&frame, &desktop->config.compositing);
}

void effects_increase_opacity(wm_desktop *desktop, wm_window *window)
{
    if (!effects_enabled(desktop) || window->opacity >= OPACITY_OPAQUE) {
        return;
    }
    uint32_t opacity = window->opacity + OPACITY_SET_STEP;
    window->opacity = opacity < OPACITY_SET_MIN ? OPACITY_OPAQUE : opacity;
    window_mark_dirty(window);
}

void effects_decrease_opacity(wm_desktop *desktop, wm_window *window)
{
    if (!effects_enabled(desktop) || window->opacity <= OPACITY_SET_MIN) {
        return;
    }
    window->opacity -= OPACITY_SET_STEP;
    window_mark_dirty(window);
}

bool effects_enabled(const wm_desktop *desktop)
{
    return desktop->config.compositing.enabled;
}

static double applied_opacity(const wm_desktop *desktop, const wm_window *window)
{
    const wm_compositing_config *config = &desktop->config.compositing;
    double opacity = (double) window->opacity / (double) OPACITY_OPAQUE;
    if (uses_move_opacity(desktop, window)) {
        opacity *= percent_factor(config->move_opacity_percent);
    }
    if (uses_resize_opacity(desktop, window)) {
        opacity *= percent_factor(config->resize_opacity_percent);
    }
    if (is_inactive(desktop, window)) {
        opacity *= percent_factor(config->inactive_opacity_percent);
    }
    return opacity;
}

static double percent_factor(int percent)
{
    return (double) percent / (double) PERCENT;
}

static bool is_inactive(const wm_desktop *desktop, const wm_window *window)
{
    return desktop->focus.focused != window && !window_is_dont_place(window)
           && !window_is_dont_focus(window);
}

static bool uses_move_opacity(const wm_desktop *desktop, const wm_window *window)
{
    return window->moving && !desktop->config.move.box_move
           && desktop->config.compositing.move_opacity_percent < PERCENT;
}

static bool uses_resize_opacity(const wm_desktop *desktop, const wm_window *window)
{
    return window->resizing && !desktop->config.move.box_resize
           && desktop->config.compositing.resize_opacity_percent < PERCENT;
}
