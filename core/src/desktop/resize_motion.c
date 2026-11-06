#include "moveresize.h"

#include "desktop_state.h"
#include "frame.h"
#include "placement.h"
#include "present.h"
#include "size_hints.h"
#include "snap.h"

static void resize_horizontally(wm_desktop *desktop, wm_window *window, uint32_t edges);
static void resize_vertically(wm_desktop *desktop, wm_window *window, uint32_t edges);
static void apply_constraints(wm_desktop *desktop, wm_window *window, uint32_t edges);
static bool is_vertical_side(wm_part part);
static bool is_horizontal_side(wm_part part);

void resize_session_motion(wm_desktop *desktop)
{
    move_session *session = &desktop->session;
    wm_window *window = session->window;
    uint32_t edges = resize_handle_edges(session->handle);
    resize_horizontally(desktop, window, edges);
    if (!window_is_shaded(window)) {
        resize_vertically(desktop, window, edges);
    }
    apply_constraints(desktop, window, edges);
    window->moving_edges = resize_moving_edges(session->handle);
    window->follow_drawn = true;
    window->follow_edges = window->moving_edges;
    window_mark_dirty(window);
    if (session->box) {
        wm_rect frame = frame_rect(desktop, window);
        present_show_outline(desktop, &frame);
    }
}

void resize_session_cancel(wm_desktop *desktop)
{
    wm_window *window = desktop->session.window;
    window->client = desktop->session.cancel_client;
    window_mark_dirty(window);
}

void resize_session_arrow(wm_desktop *desktop, wm_part side, wm_point step)
{
    wm_part handle = desktop->session.handle;
    bool same_axis = (is_vertical_side(side) && is_vertical_side(handle))
                     || (is_horizontal_side(side) && is_horizontal_side(handle));
    if (same_axis) {
        moveresize_move_pointer(desktop, step.x, step.y);
    } else {
        moveresize_set_handle(desktop, side);
    }
}

uint32_t resize_moving_edges(wm_part handle)
{
    return resize_handle_edges(handle) & (WM_EDGE_LEFT | WM_EDGE_TOP);
}

uint32_t resize_handle_edges(wm_part handle)
{
    switch (handle) {
    case WM_PART_CORNER_BOTTOM_LEFT:
        return WM_EDGE_BOTTOM | WM_EDGE_LEFT;
    case WM_PART_CORNER_BOTTOM_RIGHT:
        return WM_EDGE_BOTTOM | WM_EDGE_RIGHT;
    case WM_PART_CORNER_TOP_LEFT:
        return WM_EDGE_TOP | WM_EDGE_LEFT;
    case WM_PART_CORNER_TOP_RIGHT:
        return WM_EDGE_TOP | WM_EDGE_RIGHT;
    case WM_PART_SIDE_LEFT:
        return WM_EDGE_LEFT;
    case WM_PART_SIDE_RIGHT:
        return WM_EDGE_RIGHT;
    case WM_PART_SIDE_TOP:
        return WM_EDGE_TOP;
    case WM_PART_SIDE_BOTTOM:
        return WM_EDGE_BOTTOM;
    default:
        return WM_EDGE_NONE;
    }
}

static void resize_horizontally(wm_desktop *desktop, wm_window *window, uint32_t edges)
{
    move_session *session = &desktop->session;
    wm_extents extents = frame_extents(desktop, window);
    wm_rect *client = &window->client;
    int delta = desktop->pointer.x - session->mouse_origin.x;
    int right_edge = client->x + client->width;
    if (edges & WM_EDGE_LEFT) {
        int old_width = client->width;
        client->width = session->origin_size.width - delta;
        client->x -= client->width - old_width;
        client->x = snap_closest_edge_x(desktop, window, client->x - extents.left) + extents.left;
        client->width = right_edge - client->x;
    } else if (edges & WM_EDGE_RIGHT) {
        client->width = session->origin_size.width + delta;
        int edge = client->x + client->width + extents.right;
        client->width = snap_closest_edge_x(desktop, window, edge) - client->x - extents.right;
    }
}

static void resize_vertically(wm_desktop *desktop, wm_window *window, uint32_t edges)
{
    move_session *session = &desktop->session;
    wm_extents extents = frame_extents(desktop, window);
    wm_rect *client = &window->client;
    int delta = desktop->pointer.y - session->mouse_origin.y;
    int bottom_edge = client->y + client->height;
    if (edges & WM_EDGE_TOP) {
        int old_height = client->height;
        client->height = session->origin_size.height - delta;
        client->y -= client->height - old_height;
        client->y = snap_closest_edge_y(desktop, window, client->y - extents.top) + extents.top;
        client->height = bottom_edge - client->y;
    } else if (edges & WM_EDGE_BOTTOM) {
        client->height = session->origin_size.height + delta;
        int edge = client->y + client->height + extents.bottom;
        client->height = snap_closest_edge_y(desktop, window, edge) - client->y - extents.bottom;
    }
}

static void apply_constraints(wm_desktop *desktop, wm_window *window, uint32_t edges)
{
    wm_rect *client = &window->client;
    int right_edge = client->x + client->width;
    int bottom_edge = client->y + client->height;
    int x = client->x;
    int y = client->y;
    placement_constrain(desktop, window);
    client->height -= client->y - y;
    client->width -= client->x - x;
    size_hints_fit_user(desktop, window, edges, client);
    if (edges & WM_EDGE_LEFT) {
        client->x = right_edge - client->width;
    }
    if ((edges & WM_EDGE_TOP) && !window_is_shaded(window)) {
        client->y = bottom_edge - client->height;
    }
}

static bool is_vertical_side(wm_part part)
{
    return part == WM_PART_SIDE_TOP || part == WM_PART_SIDE_BOTTOM;
}

static bool is_horizontal_side(wm_part part)
{
    return part == WM_PART_SIDE_LEFT || part == WM_PART_SIDE_RIGHT;
}
