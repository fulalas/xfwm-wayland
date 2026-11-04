#include "moveresize.h"

#include "desktop_state.h"
#include "frame.h"
#include "maximize.h"
#include "monitors.h"
#include "placement.h"
#include "present.h"
#include "snap.h"
#include "tile.h"
#include "workspaces.h"
#include "wrap.h"

enum {
    TILE_DISTANCE = 10,
    BORDER_TILE_LENGTH_RELATIVE = 5,
};

typedef struct tile_zone {
    wm_rect monitor;
    int distance;
    int corner;
    wm_point pointer;
} tile_zone;

static void restore_size(wm_desktop *desktop, wm_window *window);
static bool tile_on_move(wm_desktop *desktop, wm_window *window, wm_point pointer);
static bool tile_edges(wm_desktop *desktop, wm_window *window, const tile_zone *zone);
static bool tile_corners(wm_desktop *desktop, wm_window *window, const tile_zone *zone);
static bool near_left(const tile_zone *zone, int limit);
static bool near_right(const tile_zone *zone, int limit);
static bool near_top(const tile_zone *zone, int limit);
static bool near_bottom(const tile_zone *zone, int limit);
static void restore_maximize(wm_desktop *desktop, wm_window *window);

void move_session_motion(wm_desktop *desktop, uint32_t time_ms)
{
    move_session *session = &desktop->session;
    wm_window *window = session->window;
    wm_point previous = {window->client.x, window->client.y};
    if (desktop->workspaces.count > 1 && !session->is_transient) {
        wrap_at_screen_edges(desktop, window, time_ms);
    }
    if (window->restore_size_pos) {
        restore_size(desktop, window);
    }
    wm_point pointer = desktop->pointer;
    window->client.x = session->window_origin.x + (pointer.x - session->mouse_origin.x);
    window->client.y = session->window_origin.y + (pointer.y - session->mouse_origin.y);
    snap_position(desktop, window, previous);
    if (!tile_on_move(desktop, window, pointer)) {
        placement_constrain(desktop, window);
    }
    window_mark_dirty(window);
    if (session->box) {
        wm_rect frame = frame_rect(desktop, window);
        present_show_outline(desktop, &frame);
    }
}

void move_session_cancel(wm_desktop *desktop)
{
    move_session *session = &desktop->session;
    wm_window *window = session->window;
    window->client = session->cancel_client;
    if (desktop->workspaces.current != session->cancel_workspace) {
        workspaces_switch_without_focus(desktop, session->cancel_workspace, window);
    }
    restore_maximize(desktop, window);
    window->restore_size_pos = session->cancel_restore;
    window->tile = session->cancel_tile;
    window->client = session->cancel_client;
    window_mark_dirty(window);
}

static void restore_size(wm_desktop *desktop, wm_window *window)
{
    move_session *session = &desktop->session;
    bool changed = maximize_toggle_in_place(desktop, window, window->states & WM_STATE_MAXIMIZED);
    if (maximize_restore_geometry(window)) {
        changed = true;
    }
    if (!changed) {
        return;
    }
    window->tile = TILE_NONE;
    wm_rect frame = frame_rect(desktop, window);
    session->window_origin = (wm_point) {window->client.x, window->client.y};
    session->mouse_origin.x = frame.x + (int) (session->pointer_ratio_x * frame.width);
    session->mouse_origin.y = frame.y + (int) (session->pointer_ratio_y * frame.height);
}

static bool tile_on_move(wm_desktop *desktop, wm_window *window, wm_point pointer)
{
    const wm_move_config *config = &desktop->config.move;
    if (!config->tile_on_move || config->wrap_windows) {
        return false;
    }
    tile_zone zone;
    zone.monitor = monitors_full_at(desktop, pointer);
    int half_title = frame_title_height(desktop) / 2;
    zone.distance = TILE_DISTANCE < half_title ? TILE_DISTANCE : half_title;
    int shortest = zone.monitor.width < zone.monitor.height ? zone.monitor.width
                                                            : zone.monitor.height;
    zone.corner = shortest / BORDER_TILE_LENGTH_RELATIVE;
    zone.pointer = pointer;
    const wm_rect *monitor = &zone.monitor;
    if (pointer.x < monitor->x - 1 || pointer.x >= wm_rect_right(monitor) + 1
        || pointer.y < monitor->y - 1 || pointer.y >= wm_rect_bottom(monitor) + 1) {
        return false;
    }
    return tile_edges(desktop, window, &zone) || tile_corners(desktop, window, &zone);
}

static bool tile_edges(wm_desktop *desktop, wm_window *window, const tile_zone *zone)
{
    wm_point p = zone->pointer;
    bool middle_vertical = !near_top(zone, zone->corner) && !near_bottom(zone, zone->corner);
    if (middle_vertical && near_left(zone, zone->distance)) {
        return tile_window_at(desktop, window, p, TILE_LEFT);
    }
    if (middle_vertical && near_right(zone, zone->distance)) {
        return tile_window_at(desktop, window, p, TILE_RIGHT);
    }
    bool middle_horizontal = !near_left(zone, zone->corner) && !near_right(zone, zone->corner);
    if (middle_horizontal && near_top(zone, zone->distance)) {
        return maximize_toggle_at(desktop, window, p, WM_STATE_MAXIMIZED);
    }
    return false;
}

static bool tile_corners(wm_desktop *desktop, wm_window *window, const tile_zone *zone)
{
    int d = zone->distance;
    int c = zone->corner;
    wm_point p = zone->pointer;
    if ((near_left(zone, c) && near_top(zone, d)) || (near_left(zone, d) && near_top(zone, c))) {
        return tile_window_at(desktop, window, p, TILE_UP_LEFT);
    }
    if ((near_right(zone, c) && near_top(zone, d)) || (near_right(zone, d) && near_top(zone, c))) {
        return tile_window_at(desktop, window, p, TILE_UP_RIGHT);
    }
    if ((near_left(zone, c) && near_bottom(zone, d))
        || (near_left(zone, d) && near_bottom(zone, c))) {
        return tile_window_at(desktop, window, p, TILE_DOWN_LEFT);
    }
    if ((near_right(zone, c) && near_bottom(zone, d))
        || (near_right(zone, d) && near_bottom(zone, c))) {
        return tile_window_at(desktop, window, p, TILE_DOWN_RIGHT);
    }
    return false;
}

static bool near_left(const tile_zone *zone, int limit)
{
    return zone->pointer.x < zone->monitor.x + limit;
}

static bool near_right(const tile_zone *zone, int limit)
{
    return zone->pointer.x >= wm_rect_right(&zone->monitor) - limit;
}

static bool near_top(const tile_zone *zone, int limit)
{
    return zone->pointer.y < zone->monitor.y + limit;
}

static bool near_bottom(const tile_zone *zone, int limit)
{
    return zone->pointer.y >= wm_rect_bottom(&zone->monitor) - limit;
}

static void restore_maximize(wm_desktop *desktop, wm_window *window)
{
    uint32_t wanted = desktop->session.cancel_states & WM_STATE_MAXIMIZED;
    uint32_t current = window->states & WM_STATE_MAXIMIZED;
    if (wanted != current) {
        maximize_toggle_in_place(desktop, window, wanted ^ current);
    }
}
