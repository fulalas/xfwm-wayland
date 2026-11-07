#include <xcb/xcb_icccm.h>
#include <wlr/xwayland/xwayland.h>

#include "x11_gravity.h"
#include "xwayland_view_private.h"

static wm_extents current_extents(const wm_window_view *state);

int xwayland_view_gravity(const xwayland_view *xv)
{
    const xcb_size_hints_t *hints = xv->xsurface->size_hints;
    if (hints != NULL && (hints->flags & XCB_ICCCM_SIZE_HINT_P_WIN_GRAVITY) != 0) {
        return (int) hints->win_gravity;
    }
    return XCB_GRAVITY_NORTH_WEST;
}

wm_rect xwayland_view_requested_rect(const xwayland_view *xv, const x11_configure *request)
{
    wm_window_view state = wm_window_current_view(xwayland_view_window(xv));
    x11_framed_window framed = {state.client, current_extents(&state)};
    return x11_gravity_configure(&framed, request);
}

wm_point xwayland_view_initial_position(const xwayland_view *xv)
{
    const struct wlr_xwayland_surface *xs = xv->xsurface;
    uint32_t capabilities = xwayland_view_capabilities(xv);
    wm_extents extents = wm_desktop_frame_extents(xv->server->desktop, capabilities);
    return x11_gravity_place((wm_point){xs->x, xs->y}, xwayland_view_gravity(xv), &extents);
}

static wm_extents current_extents(const wm_window_view *state)
{
    if (state->decor == NULL) {
        return (wm_extents){0, 0, 0, 0};
    }
    return state->decor->extents;
}
