#include <xcb/xcb_icccm.h>
#include <wlr/xwayland/xwayland.h>

#include "x11_gravity.h"
#include "x11_scale.h"
#include "xwayland_view_private.h"

static wm_extents current_extents(const wm_window_view *state);
static void remember_request(xwayland_view *xv, const x11_configure *request,
                             const x11_configure *logical, const wm_rect *client);

int xwayland_view_gravity(const xwayland_view *xv)
{
    const xcb_size_hints_t *hints = xv->xsurface->size_hints;
    if (hints != NULL && (hints->flags & XCB_ICCCM_SIZE_HINT_P_WIN_GRAVITY) != 0) {
        return (int) hints->win_gravity;
    }
    return XCB_GRAVITY_NORTH_WEST;
}

wm_rect xwayland_view_take_request(xwayland_view *xv, const x11_configure *request)
{
    wm_window_view state = wm_window_current_view(xwayland_view_window(xv));
    x11_framed_window framed = {state.client, current_extents(&state)};
    x11_configure logical = *request;
    logical.values = x11_scale_rect_from_x(xwayland_view_scale(xv), request->values);
    wm_rect client = x11_gravity_configure(&framed, &logical);
    remember_request(xv, request, &logical, &client);
    return client;
}

static void remember_request(xwayland_view *xv, const x11_configure *request,
                             const x11_configure *logical, const wm_rect *client)
{
    wm_rect *kept = &xv->known_x;
    const wm_rect *values = &request->values;
    uint32_t fields = request->fields;
    bool client_x = (fields & XCB_CONFIG_WINDOW_X) && logical->values.x == client->x;
    bool client_y = (fields & XCB_CONFIG_WINDOW_Y) && logical->values.y == client->y;
    kept->x = client_x ? values->x : kept->x;
    kept->y = client_y ? values->y : kept->y;
    kept->width = fields & XCB_CONFIG_WINDOW_WIDTH ? values->width : kept->width;
    kept->height = fields & XCB_CONFIG_WINDOW_HEIGHT ? values->height : kept->height;
}

wm_point xwayland_view_requested_position(const xwayland_view *xv)
{
    const struct wlr_xwayland_surface *xs = xv->xsurface;
    double scale = xwayland_view_scale(xv);
    return (wm_point){x11_scale_from_x(scale, xs->x), x11_scale_from_x(scale, xs->y)};
}

wm_point xwayland_view_gravity_offset(const xwayland_view *xv)
{
    uint32_t capabilities = xwayland_view_capabilities(xv);
    wm_extents extents = wm_desktop_frame_extents(xv->server->desktop, capabilities);
    return x11_gravity_place((wm_point){0, 0}, xwayland_view_gravity(xv), &extents);
}

static wm_extents current_extents(const wm_window_view *state)
{
    if (state->decor == NULL) {
        return (wm_extents){0, 0, 0, 0};
    }
    return state->decor->extents;
}
