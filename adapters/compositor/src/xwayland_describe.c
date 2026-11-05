#include <xcb/xcb_icccm.h>
#include <wlr/xwayland/xwayland.h>

#include "icon_convert.h"
#include "xwayland.h"
#include "xwayland_view_private.h"

enum {
    ICON_WANTED_SIZE = 64,
};

typedef struct type_pair {
    enum wlr_xwayland_net_wm_window_type x11;
    wm_window_type wm;
} type_pair;

static const type_pair type_pairs[] = {
    {WLR_XWAYLAND_NET_WM_WINDOW_TYPE_DESKTOP, WM_TYPE_DESKTOP},
    {WLR_XWAYLAND_NET_WM_WINDOW_TYPE_DOCK, WM_TYPE_DOCK},
    {WLR_XWAYLAND_NET_WM_WINDOW_TYPE_TOOLBAR, WM_TYPE_TOOLBAR},
    {WLR_XWAYLAND_NET_WM_WINDOW_TYPE_MENU, WM_TYPE_MENU},
    {WLR_XWAYLAND_NET_WM_WINDOW_TYPE_DROPDOWN_MENU, WM_TYPE_MENU},
    {WLR_XWAYLAND_NET_WM_WINDOW_TYPE_POPUP_MENU, WM_TYPE_MENU},
    {WLR_XWAYLAND_NET_WM_WINDOW_TYPE_COMBO, WM_TYPE_MENU},
    {WLR_XWAYLAND_NET_WM_WINDOW_TYPE_UTILITY, WM_TYPE_UTILITY},
    {WLR_XWAYLAND_NET_WM_WINDOW_TYPE_SPLASH, WM_TYPE_SPLASH},
    {WLR_XWAYLAND_NET_WM_WINDOW_TYPE_DIALOG, WM_TYPE_DIALOG},
    {WLR_XWAYLAND_NET_WM_WINDOW_TYPE_NOTIFICATION, WM_TYPE_NOTIFICATION},
    {WLR_XWAYLAND_NET_WM_WINDOW_TYPE_NORMAL, WM_TYPE_NORMAL},
};


static uint32_t current_states(const struct wlr_xwayland_surface *xsurface);
static bool surface_is_urgent(const struct wlr_xwayland_surface *xsurface);
static bool has_position(const struct wlr_xwayland_surface *xsurface);
static strut_range strut_side(uint32_t size, uint32_t start, uint32_t end);
static void set_maximized(struct wlr_xwayland_surface *xsurface, bool value);
static void set_layer_states(struct wlr_xwayland_surface *xsurface, const wm_window_view *state);

wm_window_desc xwayland_view_describe(const xwayland_view *xv)
{
    const struct wlr_xwayland_surface *xs = xv->xsurface;
    bool positioned = has_position(xs);
    wm_point position = positioned ? xwayland_view_initial_position(xv) : (wm_point){xs->x, xs->y};
    wm_window_desc desc = {
        .type = xwayland_view_type(xv),
        .capabilities = xwayland_view_capabilities(xv),
        .states = current_states(xs),
        .geometry = {position.x, position.y, xs->width, xs->height},
        .has_position = positioned,
        .size_hints = xwayland_view_size_hints(xv),
        .parent = xwayland_view_parent_window(xv),
        .title = xs->title,
        .app_id = xs->class,
        .opacity = xs->opacity,
        .activation_allowed = true,
    };
    return desc;
}

uint32_t xwayland_view_capabilities(const xwayland_view *xv)
{
    uint32_t capabilities = WM_CAP_ALL;
    if (xv->xsurface->decorations != WLR_XWAYLAND_SURFACE_DECORATIONS_ALL) {
        capabilities &= ~(uint32_t) WM_CAP_DECORATED;
    }
    return capabilities;
}

wm_window_type xwayland_view_type(const xwayland_view *xv)
{
    for (size_t i = 0; i < sizeof(type_pairs) / sizeof(type_pairs[0]); i++) {
        if (wlr_xwayland_surface_has_window_type(xv->xsurface, type_pairs[i].x11)) {
            return type_pairs[i].wm;
        }
    }
    return WM_TYPE_UNSPECIFIED;
}

wm_size_hints xwayland_view_size_hints(const xwayland_view *xv)
{
    wm_size_hints hints = {{0, 0}, {0, 0}};
    const xcb_size_hints_t *size_hints = xv->xsurface->size_hints;
    if (size_hints == NULL) {
        return hints;
    }
    if ((size_hints->flags & XCB_ICCCM_SIZE_HINT_P_MIN_SIZE) != 0) {
        hints.min = (wm_size){size_hints->min_width, size_hints->min_height};
    }
    if ((size_hints->flags & XCB_ICCCM_SIZE_HINT_P_MAX_SIZE) != 0) {
        hints.max = (wm_size){size_hints->max_width, size_hints->max_height};
    }
    return hints;
}

wm_window *xwayland_view_parent_window(const xwayland_view *xv)
{
    struct wlr_xwayland_surface *parent = xv->xsurface->parent;
    if (parent == NULL || parent->data == NULL) {
        return NULL;
    }
    return xwayland_view_window(parent->data);
}

bool xwayland_view_is_urgent(const xwayland_view *xv)
{
    return surface_is_urgent(xv->xsurface);
}

void xwayland_view_fetch_icon(xwayland_view *xv)
{
    xcb_ewmh_get_wm_icon_reply_t reply;
    cairo_surface_t *icon = NULL;
    if (wlr_xwayland_surface_fetch_icon(xv->xsurface, &reply)) {
        icon = icon_convert_ewmh(&reply, ICON_WANTED_SIZE);
        xcb_ewmh_get_wm_icon_reply_wipe(&reply);
    }
    if (icon == NULL) {
        icon_slot_lookup_app(xv->icon, xv->xsurface->class);
        return;
    }
    icon_slot_set(xv->icon, icon);
    cairo_surface_destroy(icon);
}

strut xwayland_view_strut(const xwayland_view *xv)
{
    strut s = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
    const xcb_ewmh_wm_strut_partial_t *p = xv->xsurface->strut_partial;
    if (xv->view == NULL || p == NULL) {
        return s;
    }
    s.left = strut_side(p->left, p->left_start_y, p->left_end_y);
    s.right = strut_side(p->right, p->right_start_y, p->right_end_y);
    s.top = strut_side(p->top, p->top_start_x, p->top_end_x);
    s.bottom = strut_side(p->bottom, p->bottom_start_x, p->bottom_end_x);
    return s;
}

void xwayland_view_send_states(void *self, const wm_window_view *state)
{
    xwayland_view *xv = self;
    struct wlr_xwayland_surface *xs = xv->xsurface;
    set_maximized(xs, state->maximized);
    if (xs->fullscreen != state->fullscreen) {
        wlr_xwayland_surface_set_fullscreen(xs, state->fullscreen);
    }
    if (xs->minimized != state->minimized) {
        wlr_xwayland_surface_set_minimized(xs, state->minimized);
    }
    if (xs->sticky != state->sticky) {
        wlr_xwayland_surface_set_sticky(xs, state->sticky);
    }
    if (xs->shaded != state->shaded) {
        wlr_xwayland_surface_set_shaded(xs, state->shaded);
    }
    set_layer_states(xs, state);
    x11_window_target target = {xwayland_shell_link(xv->server->xwayland), xs->window_id};
    x11_window_props_publish(&xv->published, &target, state);
}

static uint32_t current_states(const struct wlr_xwayland_surface *xs)
{
    uint32_t states = 0;
    states |= xs->maximized_horz ? WM_STATE_MAXIMIZED_HORZ : 0;
    states |= xs->maximized_vert ? WM_STATE_MAXIMIZED_VERT : 0;
    states |= xs->fullscreen ? WM_STATE_FULLSCREEN : 0;
    states |= xs->minimized ? WM_STATE_MINIMIZED : 0;
    states |= xs->shaded ? WM_STATE_SHADED : 0;
    states |= xs->sticky ? WM_STATE_STICKY : 0;
    states |= xs->above ? WM_STATE_ABOVE : 0;
    states |= xs->below ? WM_STATE_BELOW : 0;
    states |= xs->skip_taskbar ? WM_STATE_SKIP_TASKBAR : 0;
    states |= xs->skip_pager ? WM_STATE_SKIP_PAGER : 0;
    states |= xs->modal ? WM_STATE_MODAL : 0;
    states |= surface_is_urgent(xs) ? WM_STATE_URGENT : 0;
    return states;
}

static bool surface_is_urgent(const struct wlr_xwayland_surface *xs)
{
    bool hinted = xs->hints != NULL && xcb_icccm_wm_hints_get_urgency(xs->hints) != 0;
    return hinted || xs->demands_attention;
}

static bool has_position(const struct wlr_xwayland_surface *xs)
{
    const uint32_t position_flags =
        XCB_ICCCM_SIZE_HINT_US_POSITION | XCB_ICCCM_SIZE_HINT_P_POSITION;
    return xs->size_hints != NULL && (xs->size_hints->flags & position_flags) != 0;
}

static strut_range strut_side(uint32_t size, uint32_t start, uint32_t end)
{
    return (strut_range){(int) size, (int) start, (int) end};
}

static void set_maximized(struct wlr_xwayland_surface *xs, bool value)
{
    if (xs->maximized_horz != value || xs->maximized_vert != value) {
        wlr_xwayland_surface_set_maximized(xs, value, value);
    }
}

static void set_layer_states(struct wlr_xwayland_surface *xs, const wm_window_view *state)
{
    if (xs->above != state->above) {
        wlr_xwayland_surface_set_above(xs, state->above);
    }
    if (xs->below != state->below) {
        wlr_xwayland_surface_set_below(xs, state->below);
    }
}
