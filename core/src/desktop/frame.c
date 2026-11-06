#include "frame.h"

#include <string.h>

#include "desktop_state.h"
#include "monitors.h"

static void fill_capabilities(const wm_window *window, wm_decor_window *decor);
static void fill_states(const wm_window *window, wm_decor_window *decor);
static bool frame_is_drawn_active(const wm_desktop *desktop, const wm_window *window);

wm_decor_window frame_decor_window(const wm_desktop *desktop, const wm_window *window)
{
    wm_decor_window decor;
    memset(&decor, 0, sizeof(decor));
    wm_rect shown = window_shown_client(window);
    decor.client_size = (wm_size) {shown.width, shown.height};
    decor.state = frame_is_drawn_active(desktop, window) ? WM_FRAME_ACTIVE : WM_FRAME_INACTIVE;
    for (int i = 0; i < WM_BUTTON_COUNT; i++) {
        decor.buttons[i] = window->buttons[i];
    }
    fill_capabilities(window, &decor);
    fill_states(window, &decor);
    decor.has_icon = window->has_icon && desktop->config.frame.show_app_icon;
    return decor;
}

wm_extents frame_extents(const wm_desktop *desktop, const wm_window *window)
{
    wm_decor_window decor = frame_decor_window(desktop, window);
    wm_decor_input input = {&desktop->theme, &desktop->config.frame, &decor};
    return wm_decor_extents(&input);
}

wm_extents wm_desktop_frame_extents(const wm_desktop *desktop, uint32_t capabilities)
{
    wm_decor_window decor;
    memset(&decor, 0, sizeof(decor));
    decor.has_border = (capabilities & WM_CAP_DECORATED) != 0;
    wm_decor_input input = {&desktop->theme, &desktop->config.frame, &decor};
    return wm_decor_extents(&input);
}

wm_size wm_desktop_client_bounds(const wm_desktop *desktop, uint32_t capabilities)
{
    wm_rect area = monitors_work_area_at(desktop, monitors_pointer(desktop));
    wm_extents frame = wm_desktop_frame_extents(desktop, capabilities);
    wm_rect client = wm_rect_shrink(&area, &frame);
    return (wm_size) {client.width, client.height};
}

wm_rect frame_rect(const wm_desktop *desktop, const wm_window *window)
{
    return frame_rect_for_client(desktop, window, &window->client);
}

wm_point frame_center(const wm_desktop *desktop, const wm_window *window)
{
    wm_rect frame = frame_rect(desktop, window);
    return wm_rect_center(&frame);
}

wm_rect frame_rect_for_client(const wm_desktop *desktop, const wm_window *window,
                              const wm_rect *client)
{
    wm_extents extents = frame_extents(desktop, window);
    wm_rect frame = wm_rect_grow(client, &extents);
    if (window_is_shaded(window) && frame_is_shown(desktop, window)) {
        frame.height = extents.top + extents.bottom;
    }
    return frame;
}

bool frame_is_shown(const wm_desktop *desktop, const wm_window *window)
{
    const wm_frame_config *config = &desktop->config.frame;
    if (!window_has_flag(window, WINDOW_HAS_BORDER) || window_is_fullscreen(window)) {
        return false;
    }
    bool hides_title = (window_has_state(window, WM_STATE_HIDE_TITLEBAR)
                        || config->titleless_maximize)
                       && config->borderless_maximize;
    return window_is_shaded(window) || !window_is_maximized(window) || !hides_title;
}

int frame_title_height(const wm_desktop *desktop)
{
    return wm_decor_title_height(&desktop->theme);
}

static void fill_capabilities(const wm_window *window, wm_decor_window *decor)
{
    decor->has_border = window_has_flag(window, WINDOW_HAS_BORDER);
    decor->has_menu = window_has_flag(window, WINDOW_HAS_MENU);
    decor->has_stick = window_has_flag(window, WINDOW_HAS_STICK);
    decor->can_hide = window_can_hide(window);
    decor->can_maximize = window_can_maximize(window);
    decor->has_close = window_has_flag(window, WINDOW_HAS_CLOSE);
}

static void fill_states(const wm_window *window, wm_decor_window *decor)
{
    decor->maximized_horizontally = window_has_state(window, WM_STATE_MAXIMIZED_HORZ);
    decor->maximized_vertically = window_has_state(window, WM_STATE_MAXIMIZED_VERT);
    decor->fullscreen = window_is_fullscreen(window);
    decor->shaded = window_is_shaded(window);
    decor->sticky = window_is_sticky(window);
    decor->hide_titlebar = window_has_state(window, WM_STATE_HIDE_TITLEBAR);
}

static bool frame_is_drawn_active(const wm_desktop *desktop, const wm_window *window)
{
    if (desktop->focus.focused == window) {
        return true;
    }
    return window_has_state(window, WM_STATE_URGENT) && window->seen_active;
}
