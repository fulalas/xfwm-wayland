#include <wlr/xwayland/xwayland.h>

#include "ewmh_payload.h"
#include "output.h"
#include "xwayland_ewmh.h"
#include "xwayland_shell_private.h"
#include "xwayland_view_private.h"

enum {
    EVENT_TYPE_MASK = 0x7f,
    MESSAGE_FORMAT = 32,
    GRAVITY_MASK = 0xff,
    FIELDS_SHIFT = 8,
    FIELDS_MASK = 0xf,
    MOVERESIZE_SIZE_KEYBOARD = 9,
    MOVERESIZE_MOVE_KEYBOARD = 10,
    FULLSCREEN_MONITOR_SIDES = 4,
};

typedef void (*message_handler)(xwayland_shell *shell, const xcb_client_message_event_t *message);

typedef struct message_route {
    x11_atom_id type;
    message_handler handle;
} message_route;

static void switch_desktop(xwayland_shell *shell, const xcb_client_message_event_t *message);
static void move_to_desktop(xwayland_shell *shell, const xcb_client_message_event_t *message);
static void move_resize(xwayland_shell *shell, const xcb_client_message_event_t *message);
static void request_extents(xwayland_shell *shell, const xcb_client_message_event_t *message);
static void show_window_menu(xwayland_shell *shell, const xcb_client_message_event_t *message);
static void close_window(xwayland_shell *shell, const xcb_client_message_event_t *message);
static void set_fullscreen_monitors(xwayland_shell *shell,
                                    const xcb_client_message_event_t *message);
static bool keyboard_move_resize(xwayland_shell *shell, const xcb_client_message_event_t *message);
static xwayland_view *find_view(const xwayland_shell *shell, xcb_window_t window);
static wm_window *find_window(const xwayland_shell *shell, xcb_window_t window);

static const message_route routes[] = {
    {ATOM_NET_CURRENT_DESKTOP, switch_desktop},
    {ATOM_NET_WM_DESKTOP, move_to_desktop},
    {ATOM_NET_MOVERESIZE_WINDOW, move_resize},
    {ATOM_NET_REQUEST_FRAME_EXTENTS, request_extents},
    {ATOM_GTK_SHOW_WINDOW_MENU, show_window_menu},
    {ATOM_NET_CLOSE_WINDOW, close_window},
    {ATOM_NET_WM_FULLSCREEN_MONITORS, set_fullscreen_monitors},
};

bool xwayland_messages_handle(xwayland_shell *shell, xcb_generic_event_t *event)
{
    if ((event->response_type & EVENT_TYPE_MASK) != XCB_CLIENT_MESSAGE || !shell->link.ready) {
        return false;
    }
    const xcb_client_message_event_t *message = (const xcb_client_message_event_t *) event;
    if (message->type == x11_link_atom(&shell->link, ATOM_NET_WM_MOVERESIZE)) {
        return keyboard_move_resize(shell, message);
    }
    for (size_t i = 0; i < sizeof(routes) / sizeof(routes[0]); i++) {
        if (message->type == x11_link_atom(&shell->link, routes[i].type)) {
            routes[i].handle(shell, message);
            return true;
        }
    }
    return false;
}

static void switch_desktop(xwayland_shell *shell, const xcb_client_message_event_t *message)
{
    uint32_t desktop = message->data.data32[0];
    if (message->format == MESSAGE_FORMAT && xwayland_ewmh_can_switch_to(shell->ewmh, desktop)) {
        wm_desktop_switch_workspace(shell->server->desktop, (int) desktop);
    }
}

static void move_to_desktop(xwayland_shell *shell, const xcb_client_message_event_t *message)
{
    wm_window *window = find_window(shell, message->window);
    uint32_t desktop = message->data.data32[0];
    if (window == NULL || message->format != MESSAGE_FORMAT) {
        return;
    }
    if (desktop == EWMH_ALL_DESKTOPS) {
        wm_state_change sticky = {WM_STATE_STICKY, 0};
        wm_request_state(shell->server->desktop, window, &sticky);
    } else if (desktop < WM_WORKSPACE_MAX) {
        wm_request_workspace(shell->server->desktop, window, (int) desktop);
    }
}

static void move_resize(xwayland_shell *shell, const xcb_client_message_event_t *message)
{
    xwayland_view *xv = find_view(shell, message->window);
    wm_window *window = xv != NULL ? xwayland_view_window(xv) : NULL;
    if (window == NULL || message->format != MESSAGE_FORMAT) {
        return;
    }
    const uint32_t *data = message->data.data32;
    int gravity = (int) (data[0] & GRAVITY_MASK);
    x11_configure request = {
        .values = {(int32_t) data[1], (int32_t) data[2], (int32_t) data[3], (int32_t) data[4]},
        .fields = (data[0] >> FIELDS_SHIFT) & FIELDS_MASK,
        .gravity = gravity != 0 ? gravity : xwayland_view_gravity(xv),
    };
    wm_rect client = xwayland_view_requested_rect(xv, &request);
    wm_request_configure(shell->server->desktop, window, &client);
}

static void request_extents(xwayland_shell *shell, const xcb_client_message_event_t *message)
{
    xwayland_view *xv = find_view(shell, message->window);
    uint32_t capabilities = xv != NULL ? xwayland_view_capabilities(xv) : WM_CAP_ALL;
    wm_extents estimate = wm_desktop_frame_extents(shell->server->desktop, capabilities);
    ewmh_frame_extents extents = ewmh_frame_extents_from(&estimate);
    x11_window_target target = {&shell->link, message->window};
    x11_window_props_publish_extents(&target, &extents);
}

static void show_window_menu(xwayland_shell *shell, const xcb_client_message_event_t *message)
{
    wm_window *window = find_window(shell, message->window);
    if (window == NULL || message->format != MESSAGE_FORMAT) {
        return;
    }
    wm_rect client = wm_window_current_view(window).client;
    wm_point root = {(int32_t) message->data.data32[1], (int32_t) message->data.data32[2]};
    wm_point inside = {root.x - client.x, root.y - client.y};
    wm_request_window_menu(shell->server->desktop, window, inside);
}

static void close_window(xwayland_shell *shell, const xcb_client_message_event_t *message)
{
    wm_window *window = find_window(shell, message->window);
    if (window != NULL) {
        wm_request_close(shell->server->desktop, window);
    }
}

static void set_fullscreen_monitors(xwayland_shell *shell,
                                    const xcb_client_message_event_t *message)
{
    wm_window *window = find_window(shell, message->window);
    if (window == NULL || message->format != MESSAGE_FORMAT) {
        return;
    }
    wm_rect area = {0, 0, 0, 0};
    for (int i = 0; i < FULLSCREEN_MONITOR_SIDES; i++) {
        output *o = outputs_at_index(shell->server->outputs, message->data.data32[i]);
        if (o == NULL) {
            return;
        }
        wm_rect full = output_full(o);
        area = i == 0 ? full : wm_rect_union(&area, &full);
    }
    x11_property property = {message->window,
                             x11_link_atom(&shell->link, ATOM_NET_WM_FULLSCREEN_MONITORS),
                             XCB_ATOM_CARDINAL};
    x11_values values = {message->data.data32, FULLSCREEN_MONITOR_SIDES};
    x11_link_set_cardinals(&shell->link, &property, values);
    wm_window_set_fullscreen_area(shell->server->desktop, window, &area);
}

static bool keyboard_move_resize(xwayland_shell *shell, const xcb_client_message_event_t *message)
{
    uint32_t direction = message->data.data32[2];
    if (direction != MOVERESIZE_SIZE_KEYBOARD && direction != MOVERESIZE_MOVE_KEYBOARD) {
        return false;
    }
    wm_window *window = find_window(shell, message->window);
    if (window == NULL) {
        return true;
    }
    if (direction == MOVERESIZE_MOVE_KEYBOARD) {
        wm_request_keyboard_move(shell->server->desktop, window);
    } else {
        wm_request_keyboard_resize(shell->server->desktop, window);
    }
    return true;
}

static xwayland_view *find_view(const xwayland_shell *shell, xcb_window_t window)
{
    xwayland_view *xv;
    wl_list_for_each(xv, &shell->views, link) {
        if (xv->xsurface->window_id == window) {
            return xv;
        }
    }
    return NULL;
}

static wm_window *find_window(const xwayland_shell *shell, xcb_window_t window)
{
    xwayland_view *xv = find_view(shell, window);
    return xv != NULL ? xwayland_view_window(xv) : NULL;
}
