#include "ewmh_payload.h"

#include <string.h>

typedef struct action_atom {
    uint32_t action;
    x11_atom_id atom;
} action_atom;

static const action_atom action_atoms[] = {
    {WM_ALLOW_MOVE, ATOM_NET_WM_ACTION_MOVE},
    {WM_ALLOW_RESIZE, ATOM_NET_WM_ACTION_RESIZE},
    {WM_ALLOW_MINIMIZE, ATOM_NET_WM_ACTION_MINIMIZE},
    {WM_ALLOW_SHADE, ATOM_NET_WM_ACTION_SHADE},
    {WM_ALLOW_STICK, ATOM_NET_WM_ACTION_STICK},
    {WM_ALLOW_MAXIMIZE, ATOM_NET_WM_ACTION_MAXIMIZE_HORZ},
    {WM_ALLOW_MAXIMIZE, ATOM_NET_WM_ACTION_MAXIMIZE_VERT},
    {WM_ALLOW_FULLSCREEN, ATOM_NET_WM_ACTION_FULLSCREEN},
    {WM_ALLOW_CHANGE_WORKSPACE, ATOM_NET_WM_ACTION_CHANGE_DESKTOP},
    {WM_ALLOW_CLOSE, ATOM_NET_WM_ACTION_CLOSE},
    {WM_ALLOW_ABOVE, ATOM_NET_WM_ACTION_ABOVE},
    {WM_ALLOW_BELOW, ATOM_NET_WM_ACTION_BELOW},
};

static void append_name(ewmh_desktop_names *names, const char *name);
static wm_extents edge_margins(const wm_rect *root, const wm_output_info *output);
static wm_extents widest(wm_extents a, wm_extents b);

uint32_t ewmh_window_desktop(const wm_window_view *view)
{
    return view->sticky ? EWMH_ALL_DESKTOPS : (uint32_t) view->workspace;
}

ewmh_frame_extents ewmh_frame_extents_from(const wm_extents *extents)
{
    int sides[EWMH_EXTENT_COUNT] = {extents->left, extents->right, extents->top, extents->bottom};
    ewmh_frame_extents result;
    for (size_t i = 0; i < EWMH_EXTENT_COUNT; i++) {
        result.values[i] = sides[i] > 0 ? (uint32_t) sides[i] : 0;
    }
    return result;
}

ewmh_action_list ewmh_actions(uint32_t allowed_actions)
{
    ewmh_action_list list = {.count = 0};
    for (size_t i = 0; i < sizeof(action_atoms) / sizeof(action_atoms[0]); i++) {
        if (allowed_actions & action_atoms[i].action) {
            list.atoms[list.count++] = action_atoms[i].atom;
        }
    }
    return list;
}

ewmh_desktop_names ewmh_names(const wm_workspaces_view *view)
{
    ewmh_desktop_names names = {.length = 0};
    for (int i = 0; i < view->count && i < WM_WORKSPACE_MAX; i++) {
        append_name(&names, view->names[i] != NULL ? view->names[i] : "");
    }
    return names;
}

wm_rect ewmh_workarea(const ewmh_screen *screen)
{
    wm_extents margins = {0, 0, 0, 0};
    for (size_t i = 0; i < screen->count; i++) {
        margins = widest(margins, edge_margins(&screen->root, &screen->outputs[i]));
    }
    return wm_rect_shrink(&screen->root, &margins);
}

static void append_name(ewmh_desktop_names *names, const char *name)
{
    size_t room = sizeof(names->bytes) - names->length;
    size_t length = strnlen(name, WM_WORKSPACE_NAME_MAX - 1);
    if (length + 1 > room) {
        return;
    }
    memcpy(names->bytes + names->length, name, length);
    names->bytes[names->length + length] = '\0';
    names->length += length + 1;
}

static wm_extents edge_margins(const wm_rect *root, const wm_output_info *output)
{
    const wm_rect *full = &output->full;
    const wm_rect *usable = &output->usable;
    wm_extents margins = {0, 0, 0, 0};
    if (full->x == root->x) {
        margins.left = usable->x - full->x;
    }
    if (wm_rect_right(full) == wm_rect_right(root)) {
        margins.right = wm_rect_right(full) - wm_rect_right(usable);
    }
    if (full->y == root->y) {
        margins.top = usable->y - full->y;
    }
    if (wm_rect_bottom(full) == wm_rect_bottom(root)) {
        margins.bottom = wm_rect_bottom(full) - wm_rect_bottom(usable);
    }
    return margins;
}

static wm_extents widest(wm_extents a, wm_extents b)
{
    return (wm_extents){wm_max(a.left, b.left), wm_max(a.right, b.right),
                        wm_max(a.top, b.top), wm_max(a.bottom, b.bottom)};
}
