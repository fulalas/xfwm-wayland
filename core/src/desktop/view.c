#include "view.h"

#include <string.h>

#include "allowed_actions.h"
#include "desktop_state.h"
#include "effects.h"
#include "frame.h"

static void refresh_decor(wm_desktop *desktop, wm_window *window);
static wm_decor_window decor_inputs(wm_desktop *desktop, wm_window *window);
static void measure_title(wm_desktop *desktop, wm_window *window);
static void fill_states(const wm_window *window, wm_window_view *view);
static wm_rect restore_client_of(const wm_window *window);
static void fill_identity(const wm_window *window, wm_window_view *view);
static unsigned workspace_id_of(const wm_desktop *desktop, const wm_window *window);
static uint32_t tile_edges(tile_mode tile);
static uint32_t view_tiled_edges(const wm_window *window);

void view_refresh(wm_desktop *desktop, wm_window *window)
{
    refresh_decor(desktop, window);
    wm_window_view *view = &window->view;
    view->client = window->frozen ? window->frozen_client : window->client;
    view->frame = frame_rect_for_client(desktop, window, &view->client);
    view->restore_client = restore_client_of(window);
    view->visible = window->visible;
    view->activated = desktop->focus.focused == window;
    fill_states(window, view);
    view->tiled_edges = view_tiled_edges(window);
    view->workspace = window->workspace;
    view->workspace_id = workspace_id_of(desktop, window);
    view->layer = window->layer;
    view->has_shadow = effects_has_shadow(desktop, window);
    view->shadow = effects_shadow_rect(desktop, window);
    view->opacity = effects_opacity(desktop, window);
    view->decor = &window->layout;
    view->decor_serial = window->decor_serial;
    fill_identity(window, view);
    view->allowed_actions = allowed_actions_of(window);
}

void view_invalidate_decor(wm_desktop *desktop)
{
    for (size_t i = 0; i < desktop->stack.count; i++) {
        desktop->stack.items[i]->decor_valid = false;
        desktop->stack.items[i]->title_measured = false;
        window_mark_dirty(desktop->stack.items[i]);
    }
}

static void refresh_decor(wm_desktop *desktop, wm_window *window)
{
    wm_decor_window inputs = decor_inputs(desktop, window);
    if (window->decor_valid && memcmp(&inputs, &window->decor_inputs, sizeof(inputs)) == 0) {
        return;
    }
    window->decor_inputs = inputs;
    wm_decor_input input = {&desktop->theme, &desktop->config.frame, &window->decor_inputs};
    wm_decor_layout_compute(&input, &window->layout);
    window->decor_valid = true;
    window->decor_serial++;
}

static wm_decor_window decor_inputs(wm_desktop *desktop, wm_window *window)
{
    wm_decor_window inputs = frame_decor_window(desktop, window);
    measure_title(desktop, window);
    inputs.title_text_size = window->title_size;
    return inputs;
}

static void measure_title(wm_desktop *desktop, wm_window *window)
{
    if (window->title_measured) {
        return;
    }
    const wm_environment *environment = &desktop->ports.environment;
    window->title_size = environment->measure_title(environment->self,
                                                    window->title ? window->title : "");
    window->title_measured = true;
}

static void fill_states(const wm_window *window, wm_window_view *view)
{
    view->maximized = window_is_maximized(window);
    view->fullscreen = window_is_fullscreen(window);
    view->minimized = window_is_minimized(window);
    view->shaded = window_is_shaded(window);
    view->sticky = window_is_sticky(window);
    view->above = window_has_state(window, WM_STATE_ABOVE);
    view->below = window_has_state(window, WM_STATE_BELOW);
    view->urgent = window_has_state(window, WM_STATE_URGENT);
    view->skip_taskbar = window_has_state(window, WM_STATE_SKIP_TASKBAR);
    view->skip_pager = window_has_state(window, WM_STATE_SKIP_PAGER);
    view->moving = window->moving;
    view->resizing = window->resizing;
}

static wm_rect restore_client_of(const wm_window *window)
{
    bool maximized_saved = window_is_maximized(window) && window->restore_size_pos;
    if (maximized_saved) {
        return window->saved;
    }
    return window_is_fullscreen(window) ? window->pre_fullscreen : window->client;
}

static void fill_identity(const wm_window *window, wm_window_view *view)
{
    view->title = window->title;
    view->app_id = window->app_id;
    view->parent = window->parent != NULL ? window->parent->handle : NULL;
}

static unsigned workspace_id_of(const wm_desktop *desktop, const wm_window *window)
{
    const workspace_state *workspaces = &desktop->workspaces;
    bool known = window->workspace >= 0 && window->workspace < workspaces->count;
    return known ? workspaces->ids[window->workspace] : 0;
}

static uint32_t tile_edges(tile_mode tile)
{
    switch (tile) {
    case TILE_LEFT:
        return WM_EDGE_LEFT | WM_EDGE_TOP | WM_EDGE_BOTTOM;
    case TILE_RIGHT:
        return WM_EDGE_RIGHT | WM_EDGE_TOP | WM_EDGE_BOTTOM;
    case TILE_UP:
        return WM_EDGE_TOP | WM_EDGE_LEFT | WM_EDGE_RIGHT;
    case TILE_DOWN:
        return WM_EDGE_BOTTOM | WM_EDGE_LEFT | WM_EDGE_RIGHT;
    case TILE_UP_LEFT:
        return WM_EDGE_TOP | WM_EDGE_LEFT;
    case TILE_UP_RIGHT:
        return WM_EDGE_TOP | WM_EDGE_RIGHT;
    case TILE_DOWN_LEFT:
        return WM_EDGE_BOTTOM | WM_EDGE_LEFT;
    case TILE_DOWN_RIGHT:
        return WM_EDGE_BOTTOM | WM_EDGE_RIGHT;
    default:
        return 0;
    }
}

static uint32_t view_tiled_edges(const wm_window *window)
{
    uint32_t edges = tile_edges(window->tile);
    if (window_has_state(window, WM_STATE_MAXIMIZED_HORZ)) {
        edges |= WM_EDGE_LEFT | WM_EDGE_RIGHT;
    }
    if (window_has_state(window, WM_STATE_MAXIMIZED_VERT)) {
        edges |= WM_EDGE_TOP | WM_EDGE_BOTTOM;
    }
    return edges;
}
