#ifndef COMPOSITOR_SEAT_H
#define COMPOSITOR_SEAT_H

#include <stdint.h>

#include "server.h"
#include "view.h"
#include "wm/geometry.h"
#include "wm/views.h"

struct wlr_seat;
struct wlr_surface;
struct wlr_input_device;
struct wlr_xcursor_manager;

typedef enum input_route {
    INPUT_ROUTE_NORMAL,
    INPUT_ROUTE_DRAG,
    INPUT_ROUTE_LOCKED,
} input_route;

seat *seat_create(compositor *server);
void seat_destroy(seat *seat);
struct wlr_seat *seat_wlr(const seat *seat);
void seat_reload_prefs(seat *seat);
input_route seat_input_route(const seat *seat);

void seat_focus_view(seat *seat, view *view);
view *seat_focused_view(const seat *seat);
void seat_focus_layer(seat *seat, struct wlr_surface *surface);
void seat_focus_unmanaged(seat *seat, struct wlr_surface *surface);
void seat_focus_clicked_view(seat *seat);
void seat_surface_gone(seat *seat, struct wlr_surface *surface);
void seat_view_destroyed(seat *seat, view *view);
void seat_focus_refresh(seat *seat);
void seat_release_pointer_grab(seat *seat);

void seat_set_core_cursor(seat *seat, wm_cursor_shape shape);
void seat_set_busy(seat *seat, bool busy);
void seat_refresh_cursor(seat *seat);
void seat_warp(seat *seat, wm_point point);
wm_point seat_pointer_position(const seat *seat);
uint32_t seat_modifiers(const seat *seat);
void seat_notify_activity(seat *seat);
struct wlr_xcursor_manager *seat_xcursors(const seat *seat);

#endif
