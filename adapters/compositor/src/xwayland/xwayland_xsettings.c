#include "xwayland_xsettings.h"

#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <wlr/util/log.h>

#include "output/output.h"
#include "xresources.h"
#include "xsettings_table.h"
#include "xsettings_wire.h"

enum {
    EVENT_TYPE_MASK = 0x7f,
    RESOURCES_MAX_WORDS = 1 << 16,
    BYTE_FORMAT = 8,
};

struct xwayland_xsettings {
    compositor *server;
    const x11_link *link;
    xsettings_table *table;
    uint32_t serial;
    xcb_window_t window;
    bool owner;
};

static bool claim_selection(xwayland_xsettings *xsettings, xcb_connection_t *connection);
static void announce_manager(const xwayland_xsettings *xsettings, xcb_connection_t *connection);
static void collect(xwayland_xsettings *xsettings);
static void add_to_table(void *self, const x11_setting *setting);
static void publish(xwayland_xsettings *xsettings);
static void update_resources(const xwayland_xsettings *xsettings, xcb_connection_t *connection);
static char *read_resources(xcb_connection_t *connection, xcb_window_t root);

xwayland_xsettings *xwayland_xsettings_create(compositor *server, const x11_link *link)
{
    xwayland_xsettings *xsettings = calloc(1, sizeof(*xsettings));
    if (xsettings == NULL) {
        return NULL;
    }
    xsettings->server = server;
    xsettings->link = link;
    xsettings->table = xsettings_table_create();
    if (xsettings->table == NULL) {
        free(xsettings);
        return NULL;
    }
    return xsettings;
}

void xwayland_xsettings_destroy(xwayland_xsettings *xsettings)
{
    xsettings_table_destroy(xsettings->table);
    free(xsettings);
}

void xwayland_xsettings_link_ready(xwayland_xsettings *xsettings)
{
    xcb_connection_t *connection = x11_link_connection(xsettings->link);
    xsettings->owner = false;
    if (connection == NULL) {
        return;
    }
    if (!claim_selection(xsettings, connection)) {
        wlr_log(WLR_INFO, "another XSETTINGS manager runs on XWayland, leaving it in charge");
        return;
    }
    xwayland_xsettings_reload(xsettings);
}

void xwayland_xsettings_reload(xwayland_xsettings *xsettings)
{
    collect(xsettings);
    publish(xsettings);
}

bool xwayland_xsettings_handle_event(xwayland_xsettings *xsettings, xcb_generic_event_t *event)
{
    if ((event->response_type & EVENT_TYPE_MASK) != XCB_SELECTION_CLEAR) {
        return false;
    }
    const xcb_selection_clear_event_t *clear = (const xcb_selection_clear_event_t *) event;
    xcb_atom_t selection = x11_link_atom(xsettings->link, ATOM_XSETTINGS_SELECTION);
    if (!xsettings->owner || clear->owner != xsettings->window || clear->selection != selection) {
        return false;
    }
    wlr_log(WLR_INFO, "another XSETTINGS manager took over on XWayland");
    xsettings->owner = false;
    return true;
}

static bool claim_selection(xwayland_xsettings *xsettings, xcb_connection_t *connection)
{
    xcb_atom_t selection = x11_link_atom(xsettings->link, ATOM_XSETTINGS_SELECTION);
    xcb_get_selection_owner_reply_t *reply = xcb_get_selection_owner_reply(
        connection, xcb_get_selection_owner(connection, selection), NULL);
    bool taken = reply == NULL || reply->owner != XCB_WINDOW_NONE;
    free(reply);
    if (taken) {
        return false;
    }
    xsettings->window = xcb_generate_id(connection);
    xcb_create_window(connection, XCB_COPY_FROM_PARENT, xsettings->window,
                      x11_link_root(connection), -1, -1, 1, 1, 0, XCB_WINDOW_CLASS_INPUT_ONLY,
                      XCB_COPY_FROM_PARENT, 0, NULL);
    xcb_set_selection_owner(connection, xsettings->window, selection, XCB_CURRENT_TIME);
    announce_manager(xsettings, connection);
    xsettings->owner = true;
    return true;
}

static void announce_manager(const xwayland_xsettings *xsettings, xcb_connection_t *connection)
{
    xcb_window_t root = x11_link_root(connection);
    xcb_client_message_event_t message = {
        .response_type = XCB_CLIENT_MESSAGE,
        .format = 32,
        .window = root,
        .type = x11_link_atom(xsettings->link, ATOM_MANAGER),
        .data.data32 = {XCB_CURRENT_TIME, x11_link_atom(xsettings->link, ATOM_XSETTINGS_SELECTION),
                        xsettings->window, 0, 0},
    };
    xcb_send_event(connection, 0, root, XCB_EVENT_MASK_STRUCTURE_NOTIFY,
                   (const char *) &message);
    xcb_flush(connection);
}

static void collect(xwayland_xsettings *xsettings)
{
    const compositor_services *services = xsettings->server->services;
    x11_setting_sink sink = {xsettings->table, add_to_table};
    xsettings_table_set_scale(xsettings->table, outputs_max_scale(xsettings->server->outputs));
    xsettings_table_reset(xsettings->table);
    services->x11_settings(services->self, &sink);
}

static void add_to_table(void *self, const x11_setting *setting)
{
    xsettings_table_add(self, setting);
}

static void publish(xwayland_xsettings *xsettings)
{
    xcb_connection_t *connection = x11_link_connection(xsettings->link);
    if (!xsettings->owner || connection == NULL) {
        return;
    }
    xsettings_bytes bytes = xsettings_wire_encode(xsettings->table, ++xsettings->serial);
    if (bytes.data == NULL) {
        wlr_log(WLR_ERROR, "cannot encode the XSETTINGS for XWayland");
        return;
    }
    xcb_atom_t settings = x11_link_atom(xsettings->link, ATOM_XSETTINGS_SETTINGS);
    x11_property property = {xsettings->window, settings, settings};
    x11_link_set_bytes(xsettings->link, &property,
                       (x11_bytes){(const char *) bytes.data, (uint32_t) bytes.length});
    xsettings_bytes_free(&bytes);
    update_resources(xsettings, connection);
    x11_link_flush(xsettings->link);
}

static void update_resources(const xwayland_xsettings *xsettings, xcb_connection_t *connection)
{
    xcb_window_t root = x11_link_root(connection);
    char *existing = read_resources(connection, root);
    char *merged = xresources_merge(existing, xsettings->table);
    free(existing);
    if (merged == NULL) {
        return;
    }
    xcb_change_property(connection, XCB_PROP_MODE_REPLACE, root, XCB_ATOM_RESOURCE_MANAGER,
                        XCB_ATOM_STRING, BYTE_FORMAT, (uint32_t) strlen(merged), merged);
    g_free(merged);
}

static char *read_resources(xcb_connection_t *connection, xcb_window_t root)
{
    xcb_get_property_cookie_t cookie = xcb_get_property(
        connection, 0, root, XCB_ATOM_RESOURCE_MANAGER, XCB_ATOM_STRING, 0, RESOURCES_MAX_WORDS);
    xcb_get_property_reply_t *reply = xcb_get_property_reply(connection, cookie, NULL);
    if (reply == NULL) {
        return NULL;
    }
    char *text = strndup(xcb_get_property_value(reply),
                         (size_t) xcb_get_property_value_length(reply));
    free(reply);
    return text;
}
