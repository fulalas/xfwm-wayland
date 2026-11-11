#include "xwayland_window_props.h"

#include <string.h>

#include "x11_scale.h"

static x11_window_props props_of(const x11_window_target *target, const wm_window_view *state);
static void publish_desktop(const x11_window_target *target, uint32_t desktop);
static void publish_actions(const x11_window_target *target, uint32_t actions);

void x11_window_props_publish(x11_window_props *last, const x11_window_target *target,
                              const wm_window_view *state)
{
    if (x11_link_connection(target->link) == NULL) {
        return;
    }
    x11_window_props next = props_of(target, state);
    if (!last->published || last->desktop != next.desktop) {
        publish_desktop(target, next.desktop);
    }
    if (!last->published || memcmp(&last->extents, &next.extents, sizeof(next.extents)) != 0) {
        x11_window_props_publish_extents(target, &next.extents);
    }
    if (!last->published || last->actions != next.actions) {
        publish_actions(target, next.actions);
    }
    x11_link_flush(target->link);
    *last = next;
}

void x11_window_props_publish_extents(const x11_window_target *target,
                                      const ewmh_frame_extents *extents)
{
    xcb_atom_t atom = x11_link_atom(target->link, ATOM_NET_FRAME_EXTENTS);
    x11_property property = {target->window, atom, XCB_ATOM_CARDINAL};
    x11_values values = {extents->values, EWMH_EXTENT_COUNT};
    x11_link_set_cardinals(target->link, &property, values);
    x11_link_flush(target->link);
}

ewmh_frame_extents x11_window_props_extents(const x11_window_target *target,
                                            const wm_extents *logical)
{
    wm_extents extents = x11_scale_extents_to_x(target->scale, *logical);
    return ewmh_frame_extents_from(&extents);
}

static x11_window_props props_of(const x11_window_target *target, const wm_window_view *state)
{
    wm_extents none = {0, 0, 0, 0};
    x11_window_props props = {
        .published = true,
        .desktop = ewmh_window_desktop(state),
        .extents = x11_window_props_extents(target, state->decor ? &state->decor->extents : &none),
        .actions = state->allowed_actions,
    };
    return props;
}

static void publish_desktop(const x11_window_target *target, uint32_t desktop)
{
    xcb_atom_t atom = x11_link_atom(target->link, ATOM_NET_WM_DESKTOP);
    x11_property property = {target->window, atom, XCB_ATOM_CARDINAL};
    x11_link_set_cardinals(target->link, &property, (x11_values){&desktop, 1});
}

static void publish_actions(const x11_window_target *target, uint32_t actions)
{
    ewmh_action_list list = ewmh_actions(actions);
    xcb_atom_t atom = x11_link_atom(target->link, ATOM_NET_WM_ALLOWED_ACTIONS);
    x11_property property = {target->window, atom, XCB_ATOM_ATOM};
    x11_atom_list atoms = {list.atoms, (uint32_t) list.count};
    x11_link_set_atoms(target->link, &property, atoms);
}
