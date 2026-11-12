#include "decor_private.h"

static bool is_maximized(const wm_decor_window *window);
static bool has_side_borders(const wm_decor_input *input);
static bool hides_titlebar_when_maximized(const wm_decor_input *input);
static int frame_top(const wm_decor_input *input);

wm_extents wm_decor_extents(const wm_decor_input *input)
{
    const wm_theme_metrics *theme = input->theme;
    wm_extents extents = {0, 0, frame_top(input), 0};

    if (has_side_borders(input)) {
        extents.left = decor_pick_size(theme->side_stretch[WM_SIDE_LEFT][WM_FRAME_ACTIVE],
                                       theme->side[WM_SIDE_LEFT][WM_FRAME_ACTIVE]).width;
        extents.right = decor_pick_size(theme->side_stretch[WM_SIDE_RIGHT][WM_FRAME_ACTIVE],
                                        theme->side[WM_SIDE_RIGHT][WM_FRAME_ACTIVE]).width;
        extents.bottom = decor_pick_size(theme->side_stretch[WM_SIDE_BOTTOM][WM_FRAME_ACTIVE],
                                         theme->side[WM_SIDE_BOTTOM][WM_FRAME_ACTIVE]).height;
    }
    return extents;
}

int wm_decor_title_height(const wm_theme_metrics *theme)
{
    wm_size stretch = theme->side_stretch[WM_SIDE_TOP][WM_FRAME_ACTIVE];
    if (!wm_size_is_empty(stretch)) {
        return stretch.height;
    }
    return theme->title[DECOR_TITLE_MIDDLE][WM_FRAME_ACTIVE].height;
}

wm_size decor_pick_size(wm_size preferred, wm_size fallback)
{
    return wm_size_is_empty(preferred) ? fallback : preferred;
}

bool decor_is_borderless(const wm_decor_input *input)
{
    return is_maximized(input->window) && input->config->borderless_maximize;
}

bool wm_decor_has_frame(const wm_decor_input *input)
{
    const wm_decor_window *window = input->window;
    if (!window->has_border || window->fullscreen) {
        return false;
    }
    return window->shaded || !is_maximized(window) || !hides_titlebar_when_maximized(input);
}

int decor_border_top(const wm_decor_input *input)
{
    const wm_decor_window *window = input->window;
    if (window->has_border && !window->fullscreen && decor_is_borderless(input)) {
        return input->config->border_top;
    }
    return 0;
}

int decor_button_offset(const decor_frame *frame)
{
    if (frame->borderless) {
        return wm_max(0, frame->config->maximized_offset);
    }
    return frame->config->button_offset;
}

static bool is_maximized(const wm_decor_window *window)
{
    return window->maximized_horizontally && window->maximized_vertically;
}

static bool has_side_borders(const wm_decor_input *input)
{
    const wm_decor_window *window = input->window;
    return window->has_border && !window->fullscreen && !decor_is_borderless(input);
}

static bool hides_titlebar_when_maximized(const wm_decor_input *input)
{
    const wm_frame_config *config = input->config;
    return (input->window->hide_titlebar || config->titleless_maximize)
           && config->borderless_maximize;
}

static int frame_top(const wm_decor_input *input)
{
    if (!wm_decor_has_frame(input)) {
        return 0;
    }
    return wm_decor_title_height(input->theme) - decor_border_top(input);
}
