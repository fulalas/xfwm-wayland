#include "desktop_state.h"
#include "menu.h"
#include "monitors.h"
#include "moveresize.h"
#include "placement.h"
#include "present.h"
#include "size_hints.h"
#include "states.h"
#include "window_ops.h"

static wm_part part_for_edges(uint32_t edges);
static bool accepts_configure(const wm_window *window);

void wm_request_move(wm_desktop *desktop, wm_window *window)
{
    session_start start = {window, WM_PART_NONE, monitors_pointer(desktop), 0};
    moveresize_begin_pointer_move(desktop, &start);
    present_flush(desktop);
}

void wm_request_resize(wm_desktop *desktop, wm_window *window, uint32_t edges)
{
    session_start start = {window, part_for_edges(edges), monitors_pointer(desktop), 0};
    moveresize_begin_pointer_resize(desktop, &start);
    present_flush(desktop);
}

void wm_request_window_menu(wm_desktop *desktop, wm_window *window, wm_point client_point)
{
    wm_point point = {window->client.x + client_point.x, window->client.y + client_point.y};
    menu_show(desktop, window, point);
    present_flush(desktop);
}

void wm_request_configure(wm_desktop *desktop, wm_window *window, const wm_rect *client)
{
    if (accepts_configure(window)) {
        window->client.x = client->x;
        window->client.y = client->y;
        window->client.width = size_hints_check_width(desktop, window, client->width);
        window->client.height = size_hints_check_height(desktop, window, client->height);
        window->moving_edges = 0;
        placement_constrain(desktop, window);
        window_mark_dirty(window);
    }
    present_request_configure(window);
    present_flush(desktop);
}

void wm_request_close(wm_desktop *desktop, wm_window *window)
{
    window_ops_close(desktop, window);
    present_flush(desktop);
}

void wm_request_workspace(wm_desktop *desktop, wm_window *window, int workspace)
{
    if (workspace < 0 || workspace >= desktop->workspaces.count) {
        return;
    }
    if (window_is_sticky(window)) {
        states_unstick(desktop, window);
    }
    window_ops_send_to_workspace(desktop, window, workspace);
    present_flush(desktop);
}

static wm_part part_for_edges(uint32_t edges)
{
    bool top = edges & WM_EDGE_TOP;
    bool bottom = edges & WM_EDGE_BOTTOM;
    bool left = edges & WM_EDGE_LEFT;
    bool right = edges & WM_EDGE_RIGHT;
    if (top) {
        return left ? WM_PART_CORNER_TOP_LEFT : (right ? WM_PART_CORNER_TOP_RIGHT
                                                       : WM_PART_SIDE_TOP);
    }
    if (bottom) {
        return left ? WM_PART_CORNER_BOTTOM_LEFT : (right ? WM_PART_CORNER_BOTTOM_RIGHT
                                                          : WM_PART_SIDE_BOTTOM);
    }
    if (left) {
        return WM_PART_SIDE_LEFT;
    }
    return right ? WM_PART_SIDE_RIGHT : WM_PART_CORNER_BOTTOM_RIGHT;
}

static bool accepts_configure(const wm_window *window)
{
    return !window_has_state(window, WM_STATE_MAXIMIZED | WM_STATE_FULLSCREEN)
           && window->tile == TILE_NONE && !window->moving && !window->resizing;
}
