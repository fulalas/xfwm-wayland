#include "xwayland_ewmh.h"

#include <stdio.h>
#include <stdlib.h>
#include <wlr/types/wlr_output_layout.h>

#include "ewmh_payload.h"
#include "output.h"

enum {
    RECT_FIELDS = 4,
    ATOM_NAME_MAX = 32,
    OUTPUT_MAX = 16,
};

static const x11_atom_id supported[] = {
    ATOM_NET_NUMBER_OF_DESKTOPS,      ATOM_NET_CURRENT_DESKTOP,
    ATOM_NET_DESKTOP_NAMES,           ATOM_NET_DESKTOP_GEOMETRY,
    ATOM_NET_DESKTOP_VIEWPORT,        ATOM_NET_WORKAREA,
    ATOM_GTK_WORKAREAS,               ATOM_NET_WM_DESKTOP,
    ATOM_NET_FRAME_EXTENTS,           ATOM_NET_REQUEST_FRAME_EXTENTS,
    ATOM_NET_MOVERESIZE_WINDOW,       ATOM_GTK_SHOW_WINDOW_MENU,
    ATOM_NET_WM_ALLOWED_ACTIONS,      ATOM_NET_WM_ACTION_MOVE,
    ATOM_NET_WM_ACTION_RESIZE,        ATOM_NET_WM_ACTION_MINIMIZE,
    ATOM_NET_WM_ACTION_SHADE,         ATOM_NET_WM_ACTION_STICK,
    ATOM_NET_WM_ACTION_MAXIMIZE_HORZ, ATOM_NET_WM_ACTION_MAXIMIZE_VERT,
    ATOM_NET_WM_ACTION_FULLSCREEN,    ATOM_NET_WM_ACTION_CHANGE_DESKTOP,
    ATOM_NET_WM_ACTION_CLOSE,         ATOM_NET_WM_ACTION_ABOVE,
    ATOM_NET_WM_ACTION_BELOW,
};

typedef struct screen_state {
    wm_output_info outputs[OUTPUT_MAX];
    ewmh_screen screen;
} screen_state;

struct xwayland_ewmh {
    compositor *server;
    const x11_link *link;
    bool has_workspaces;
    uint32_t count;
    uint32_t current;
    ewmh_desktop_names names;
    xcb_atom_t desktop_workareas[WM_WORKSPACE_MAX];
    bool outputs_published;
    screen_state published;
};

static void intern_desktop_workareas(xwayland_ewmh *ewmh, xcb_connection_t *connection);
static void publish_desktops(xwayland_ewmh *ewmh);
static void publish_workareas(xwayland_ewmh *ewmh, const screen_state *state);
static void publish_monitor_workareas(xwayland_ewmh *ewmh, const screen_state *state);
static void read_screen(const compositor *server, screen_state *state);
static bool same_screen(const screen_state *a, const screen_state *b);
static x11_property root_property(const xwayland_ewmh *ewmh, x11_atom_id id, xcb_atom_t type);
static void set_repeated(const xwayland_ewmh *ewmh, x11_atom_id id, x11_values item);

xwayland_ewmh *xwayland_ewmh_create(compositor *server, const x11_link *link)
{
    xwayland_ewmh *ewmh = calloc(1, sizeof(*ewmh));
    if (ewmh != NULL) {
        ewmh->server = server;
        ewmh->link = link;
    }
    return ewmh;
}

void xwayland_ewmh_destroy(xwayland_ewmh *ewmh)
{
    free(ewmh);
}

void xwayland_ewmh_link_ready(xwayland_ewmh *ewmh)
{
    xcb_connection_t *connection = x11_link_connection(ewmh->link);
    if (connection == NULL) {
        return;
    }
    intern_desktop_workareas(ewmh, connection);
    ewmh->outputs_published = false;
    x11_property property = root_property(ewmh, ATOM_NET_SUPPORTED, XCB_ATOM_ATOM);
    uint32_t count = sizeof(supported) / sizeof(supported[0]);
    x11_link_append_atoms(ewmh->link, &property, (x11_atom_list){supported, count});
    publish_desktops(ewmh);
    xwayland_ewmh_publish_outputs(ewmh);
}

void xwayland_ewmh_publish_workspaces(xwayland_ewmh *ewmh, const wm_workspaces_view *view)
{
    uint32_t previous_count = ewmh->count;
    ewmh->has_workspaces = true;
    ewmh->count = (uint32_t) view->count;
    ewmh->current = (uint32_t) view->current;
    ewmh->names = ewmh_names(view);
    publish_desktops(ewmh);
    if (previous_count != ewmh->count) {
        ewmh->outputs_published = false;
        xwayland_ewmh_publish_outputs(ewmh);
    }
}

void xwayland_ewmh_publish_outputs(xwayland_ewmh *ewmh)
{
    if (!ewmh->has_workspaces || x11_link_connection(ewmh->link) == NULL) {
        return;
    }
    screen_state state;
    read_screen(ewmh->server, &state);
    if (ewmh->outputs_published && same_screen(&state, &ewmh->published)) {
        return;
    }
    publish_workareas(ewmh, &state);
    publish_monitor_workareas(ewmh, &state);
    ewmh->published = state;
    ewmh->outputs_published = true;
}

bool xwayland_ewmh_can_switch_to(const xwayland_ewmh *ewmh, uint32_t desktop)
{
    return ewmh->has_workspaces && desktop < ewmh->count && desktop != ewmh->current;
}

static void intern_desktop_workareas(xwayland_ewmh *ewmh, xcb_connection_t *connection)
{
    xcb_intern_atom_cookie_t cookies[WM_WORKSPACE_MAX];
    for (int i = 0; i < WM_WORKSPACE_MAX; i++) {
        char name[ATOM_NAME_MAX];
        int length = snprintf(name, sizeof(name), "_GTK_WORKAREAS_D%d", i);
        cookies[i] = xcb_intern_atom(connection, 0, (uint16_t) length, name);
    }
    for (int i = 0; i < WM_WORKSPACE_MAX; i++) {
        xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(connection, cookies[i], NULL);
        ewmh->desktop_workareas[i] = reply != NULL ? reply->atom : XCB_ATOM_NONE;
        free(reply);
    }
}

static void publish_desktops(xwayland_ewmh *ewmh)
{
    if (!ewmh->has_workspaces) {
        return;
    }
    x11_property count = root_property(ewmh, ATOM_NET_NUMBER_OF_DESKTOPS, XCB_ATOM_CARDINAL);
    x11_link_set_cardinals(ewmh->link, &count, (x11_values){&ewmh->count, 1});
    x11_property current = root_property(ewmh, ATOM_NET_CURRENT_DESKTOP, XCB_ATOM_CARDINAL);
    x11_link_set_cardinals(ewmh->link, &current, (x11_values){&ewmh->current, 1});
    xcb_atom_t utf8 = x11_link_atom(ewmh->link, ATOM_UTF8_STRING);
    x11_property names = root_property(ewmh, ATOM_NET_DESKTOP_NAMES, utf8);
    x11_bytes bytes = {ewmh->names.bytes, (uint32_t) ewmh->names.length};
    x11_link_set_bytes(ewmh->link, &names, bytes);
}

static void publish_workareas(xwayland_ewmh *ewmh, const screen_state *state)
{
    const wm_rect *root = &state->screen.root;
    uint32_t geometry[] = {(uint32_t) root->width, (uint32_t) root->height};
    x11_property property = root_property(ewmh, ATOM_NET_DESKTOP_GEOMETRY, XCB_ATOM_CARDINAL);
    x11_link_set_cardinals(ewmh->link, &property, (x11_values){geometry, 2});
    uint32_t origin[] = {(uint32_t) root->x, (uint32_t) root->y};
    set_repeated(ewmh, ATOM_NET_DESKTOP_VIEWPORT, (x11_values){origin, 2});
    wm_rect area = ewmh_workarea(&state->screen);
    uint32_t workarea[RECT_FIELDS] = {(uint32_t) area.x, (uint32_t) area.y,
                                      (uint32_t) area.width, (uint32_t) area.height};
    set_repeated(ewmh, ATOM_NET_WORKAREA, (x11_values){workarea, RECT_FIELDS});
}

static void publish_monitor_workareas(xwayland_ewmh *ewmh, const screen_state *state)
{
    uint32_t rects[OUTPUT_MAX * RECT_FIELDS];
    size_t count = state->screen.count;
    for (size_t i = 0; i < count; i++) {
        const wm_rect *usable = &state->outputs[i].usable;
        uint32_t *rect = &rects[i * RECT_FIELDS];
        rect[0] = (uint32_t) usable->x;
        rect[1] = (uint32_t) usable->y;
        rect[2] = (uint32_t) usable->width;
        rect[3] = (uint32_t) usable->height;
    }
    x11_property property = root_property(ewmh, ATOM_GTK_WORKAREAS, XCB_ATOM_CARDINAL);
    for (uint32_t i = 0; i < WM_WORKSPACE_MAX; i++) {
        property.atom = ewmh->desktop_workareas[i];
        if (i < ewmh->count) {
            x11_values values = {rects, (uint32_t) (count * RECT_FIELDS)};
            x11_link_set_cardinals(ewmh->link, &property, values);
        } else {
            x11_link_delete(ewmh->link, &property);
        }
    }
}

static void read_screen(const compositor *server, screen_state *state)
{
    struct wlr_box box = {0};
    wlr_output_layout_get_box(server->layout, NULL, &box);
    size_t count = outputs_count(server->outputs);
    count = count < OUTPUT_MAX ? count : OUTPUT_MAX;
    for (size_t i = 0; i < count; i++) {
        output *o = outputs_at_index(server->outputs, i);
        state->outputs[i] = (wm_output_info){output_full(o), output_usable(o)};
    }
    state->screen = (ewmh_screen){{box.x, box.y, box.width, box.height}, state->outputs, count};
}

static bool same_screen(const screen_state *a, const screen_state *b)
{
    if (!wm_rect_equals(&a->screen.root, &b->screen.root) || a->screen.count != b->screen.count) {
        return false;
    }
    for (size_t i = 0; i < a->screen.count; i++) {
        const wm_output_info *x = &a->outputs[i];
        const wm_output_info *y = &b->outputs[i];
        if (!wm_rect_equals(&x->full, &y->full) || !wm_rect_equals(&x->usable, &y->usable)) {
            return false;
        }
    }
    return true;
}

static x11_property root_property(const xwayland_ewmh *ewmh, x11_atom_id id, xcb_atom_t type)
{
    xcb_connection_t *connection = x11_link_connection(ewmh->link);
    xcb_window_t root = connection != NULL ? x11_link_root(connection) : XCB_WINDOW_NONE;
    return (x11_property){root, x11_link_atom(ewmh->link, id), type};
}

static void set_repeated(const xwayland_ewmh *ewmh, x11_atom_id id, x11_values item)
{
    uint32_t values[WM_WORKSPACE_MAX * RECT_FIELDS];
    uint32_t count = ewmh->count < WM_WORKSPACE_MAX ? ewmh->count : WM_WORKSPACE_MAX;
    uint32_t fields = item.count < RECT_FIELDS ? item.count : RECT_FIELDS;
    for (uint32_t i = 0; i < count * fields; i++) {
        values[i] = item.data[i % fields];
    }
    x11_property property = root_property(ewmh, id, XCB_ATOM_CARDINAL);
    x11_link_set_cardinals(ewmh->link, &property, (x11_values){values, count * fields});
}
