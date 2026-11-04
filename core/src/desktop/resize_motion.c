#include "moveresize.h"

#include "desktop_state.h"
#include "frame.h"
#include "placement.h"
#include "present.h"
#include "size_hints.h"
#include "snap.h"

typedef struct resize_edges {
    bool top;
    bool bottom;
    bool left;
    bool right;
} resize_edges;

static resize_edges edges_of(wm_part handle);
static void resize_horizontally(wm_desktop *desktop, wm_window *window, resize_edges edges);
static void resize_vertically(wm_desktop *desktop, wm_window *window, resize_edges edges);
static void apply_constraints(wm_desktop *desktop, wm_window *window, resize_edges edges);
static bool is_vertical_side(wm_part part);
static bool is_horizontal_side(wm_part part);

void resize_session_motion(wm_desktop *desktop)
{
    move_session *session = &desktop->session;
    wm_window *window = session->window;
    resize_edges edges = edges_of(session->handle);
    resize_horizontally(desktop, window, edges);
    if (!window_is_shaded(window)) {
        resize_vertically(desktop, window, edges);
    }
    apply_constraints(desktop, window, edges);
    window->moving_edges = resize_moving_edges(session->handle);
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
    resize_edges edges = edges_of(handle);
    uint32_t moving = 0;
    if (edges.left) {
        moving |= WM_EDGE_LEFT;
    }
    if (edges.top) {
        moving |= WM_EDGE_TOP;
    }
    return moving;
}

static resize_edges edges_of(wm_part handle)
{
    resize_edges edges = {false, false, false, false};
    edges.top = handle == WM_PART_CORNER_TOP_LEFT || handle == WM_PART_CORNER_TOP_RIGHT
                || handle == WM_PART_SIDE_TOP;
    edges.bottom = handle == WM_PART_CORNER_BOTTOM_LEFT || handle == WM_PART_CORNER_BOTTOM_RIGHT
                   || handle == WM_PART_SIDE_BOTTOM;
    edges.right = handle == WM_PART_CORNER_TOP_RIGHT || handle == WM_PART_CORNER_BOTTOM_RIGHT
                  || handle == WM_PART_SIDE_RIGHT;
    edges.left = handle == WM_PART_CORNER_TOP_LEFT || handle == WM_PART_CORNER_BOTTOM_LEFT
                 || handle == WM_PART_SIDE_LEFT;
    return edges;
}

static void resize_horizontally(wm_desktop *desktop, wm_window *window, resize_edges edges)
{
    move_session *session = &desktop->session;
    wm_extents extents = frame_extents(desktop, window);
    wm_rect *client = &window->client;
    int delta = desktop->pointer.x - session->mouse_origin.x;
    int right_edge = client->x + client->width;
    if (edges.left) {
        int old_width = client->width;
        client->width = session->origin_size.width - delta;
        client->x -= client->width - old_width;
        client->x = snap_closest_edge_x(desktop, window, client->x - extents.left) + extents.left;
        client->width = right_edge - client->x;
    } else if (edges.right) {
        client->width = session->origin_size.width + delta;
        int edge = client->x + client->width + extents.right;
        client->width = snap_closest_edge_x(desktop, window, edge) - client->x - extents.right;
    }
}

static void resize_vertically(wm_desktop *desktop, wm_window *window, resize_edges edges)
{
    move_session *session = &desktop->session;
    wm_extents extents = frame_extents(desktop, window);
    wm_rect *client = &window->client;
    int delta = desktop->pointer.y - session->mouse_origin.y;
    int bottom_edge = client->y + client->height;
    if (edges.top) {
        int old_height = client->height;
        client->height = session->origin_size.height - delta;
        client->y -= client->height - old_height;
        client->y = snap_closest_edge_y(desktop, window, client->y - extents.top) + extents.top;
        client->height = bottom_edge - client->y;
    } else if (edges.bottom) {
        client->height = session->origin_size.height + delta;
        int edge = client->y + client->height + extents.bottom;
        client->height = snap_closest_edge_y(desktop, window, edge) - client->y - extents.bottom;
    }
}

static void apply_constraints(wm_desktop *desktop, wm_window *window, resize_edges edges)
{
    wm_rect *client = &window->client;
    int right_edge = client->x + client->width;
    int bottom_edge = client->y + client->height;
    int x = client->x;
    int y = client->y;
    placement_constrain(desktop, window);
    client->height -= client->y - y;
    client->width -= client->x - x;
    client->width = size_hints_check_width(desktop, window, client->width);
    if (edges.left) {
        client->x = right_edge - client->width;
    }
    client->height = size_hints_check_height(desktop, window, client->height);
    if (edges.top && !window_is_shaded(window)) {
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
