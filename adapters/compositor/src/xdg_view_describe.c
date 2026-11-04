#include <wlr/types/wlr_xdg_dialog_v1.h>
#include <wlr/types/wlr_xdg_shell.h>

#include "activation.h"
#include "input_mapping.h"
#include "xdg_view_private.h"

typedef struct toplevel_flag {
    bool scheduled;
    bool wanted;
    uint32_t (*send)(struct wlr_xdg_toplevel *toplevel, bool value);
} toplevel_flag;

static wm_window_type declared_type(const xdg_view *view);
static uint32_t requested_states(const xdg_view *view);
static bool is_modal(const xdg_view *view);
static void send_flag(struct wlr_xdg_toplevel *toplevel, toplevel_flag flag);
static void set_tiled(struct wlr_xdg_toplevel *toplevel, uint32_t wm_edges);

wm_window_desc xdg_view_describe(const xdg_view *xv)
{
    struct wlr_xdg_toplevel *toplevel = xv->toplevel;
    struct wlr_box geometry = toplevel->base->geometry;
    wm_window_desc desc = {
        .type = declared_type(xv),
        .capabilities = xdg_view_capabilities(xv),
        .states = requested_states(xv),
        .geometry = {0, 0, geometry.width, geometry.height},
        .has_position = false,
        .size_hints = xdg_view_size_hints(xv),
        .parent = xdg_view_parent_window(xv),
        .title = toplevel->title,
        .app_id = toplevel->app_id,
        .opacity = 1.0,
        .activation_allowed = activation_allows_map(xv->server->activation,
                                                    toplevel->base->surface),
    };
    return desc;
}

uint32_t xdg_view_capabilities(const xdg_view *xv)
{
    uint32_t capabilities = WM_CAP_ALL;
    if (xv->decorations.mode != DECORATION_SERVER) {
        capabilities &= ~(uint32_t) WM_CAP_DECORATED;
    }
    return capabilities;
}

wm_size_hints xdg_view_size_hints(const xdg_view *xv)
{
    const struct wlr_xdg_toplevel_state *current = &xv->toplevel->current;
    wm_size_hints hints = {
        .min = {current->min_width, current->min_height},
        .max = {current->max_width, current->max_height},
    };
    return hints;
}

wm_window *xdg_view_parent_window(const xdg_view *xv)
{
    xdg_view *parent = xdg_view_from_toplevel(xv->toplevel->parent);
    return parent != NULL && parent->view != NULL ? view_window(parent->view) : NULL;
}

void xdg_view_send_states(void *self, const wm_window_view *state)
{
    xdg_view *xv = self;
    struct wlr_xdg_toplevel *toplevel = xv->toplevel;
    if (!toplevel->base->initialized) {
        return;
    }
    const struct wlr_xdg_toplevel_configure *s = &toplevel->scheduled;
    const toplevel_flag flags[] = {
        {s->activated, state->activated, wlr_xdg_toplevel_set_activated},
        {s->maximized, state->maximized, wlr_xdg_toplevel_set_maximized},
        {s->fullscreen, state->fullscreen, wlr_xdg_toplevel_set_fullscreen},
        {s->resizing, state->resizing, wlr_xdg_toplevel_set_resizing},
        {s->suspended, state->minimized || !state->visible, wlr_xdg_toplevel_set_suspended},
    };
    for (size_t i = 0; i < sizeof(flags) / sizeof(flags[0]); i++) {
        send_flag(toplevel, flags[i]);
    }
    set_tiled(toplevel, state->tiled_edges);
}

void xdg_view_modal_changed(xdg_view *xv)
{
    if (xv->view != NULL) {
        wm_state_change change = view_state_change(WM_STATE_MODAL, is_modal(xv));
        view_request_state(xv->view, &change);
    }
}

static wm_window_type declared_type(const xdg_view *xv)
{
    bool dialog = wlr_xdg_dialog_v1_try_from_wlr_xdg_toplevel(xv->toplevel) != NULL;
    return dialog ? WM_TYPE_DIALOG : WM_TYPE_UNSPECIFIED;
}

static uint32_t requested_states(const xdg_view *xv)
{
    const struct wlr_xdg_toplevel_requested *requested = &xv->toplevel->requested;
    uint32_t states = 0;
    states |= requested->maximized ? WM_STATE_MAXIMIZED : 0;
    states |= requested->fullscreen ? WM_STATE_FULLSCREEN : 0;
    states |= requested->minimized ? WM_STATE_MINIMIZED : 0;
    states |= is_modal(xv) ? WM_STATE_MODAL : 0;
    return states;
}

static bool is_modal(const xdg_view *xv)
{
    struct wlr_xdg_dialog_v1 *dialog = wlr_xdg_dialog_v1_try_from_wlr_xdg_toplevel(xv->toplevel);
    return dialog != NULL && dialog->modal;
}

static void send_flag(struct wlr_xdg_toplevel *toplevel, toplevel_flag flag)
{
    if (flag.scheduled != flag.wanted) {
        flag.send(toplevel, flag.wanted);
    }
}

static void set_tiled(struct wlr_xdg_toplevel *toplevel, uint32_t wm_edges)
{
    uint32_t tiled = input_mapping_edges_to_wlr(wm_edges);
    if (toplevel->scheduled.tiled != tiled) {
        wlr_xdg_toplevel_set_tiled(toplevel, tiled);
    }
}
