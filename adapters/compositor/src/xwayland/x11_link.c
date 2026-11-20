#include "x11_link.h"

#include <stdlib.h>
#include <string.h>
#include <wlr/xwayland/xwayland.h>
#include <xcb/shape.h>
#include <xcb/xfixes.h>

enum {
    BYTE_FORMAT = 8,
    CARDINAL_FORMAT = 32,
    X11_READS_MAX = 8,
    XFIXES_DISCONNECT_MODE_MAJOR = 6,
};

typedef struct property_change {
    const x11_property *property;
    uint8_t mode;
    x11_values values;
} property_change;

static xcb_connection_t *connect_own(const char *display_name);
static void allow_lazy_exit(xcb_connection_t *connection);
static void drop_queued_events(xcb_connection_t *connection);
static x11_values atom_values(const x11_link *link, x11_atom_list list, uint32_t *atoms);
static void change_property(const x11_link *link, const property_change *change);
static xcb_get_property_cookie_t request_read(xcb_connection_t *connection,
                                              const x11_read *read);
static void receive_read(xcb_connection_t *connection, xcb_get_property_cookie_t cookie,
                         x11_read *read);
static void store_read(x11_read *read, xcb_get_property_reply_t *reply);

void x11_link_attach(x11_link *link, const struct wlr_xwayland *wlr)
{
    x11_link_detach(link);
    link->own = connect_own(wlr->display_name);
    if (link->own == NULL || !x11_atoms_intern(&link->atoms, link->own)) {
        x11_link_detach(link);
        return;
    }
    allow_lazy_exit(link->own);
}

void x11_link_detach(x11_link *link)
{
    if (link->own != NULL) {
        xcb_disconnect(link->own);
    }
    *link = (x11_link){.own = NULL};
}

xcb_connection_t *x11_link_connection(const x11_link *link)
{
    if (link->own == NULL || xcb_connection_has_error(link->own)) {
        return NULL;
    }
    return link->own;
}

static xcb_connection_t *connect_own(const char *display_name)
{
    xcb_connection_t *connection = xcb_connect(display_name, NULL);
    if (xcb_connection_has_error(connection)) {
        xcb_disconnect(connection);
        return NULL;
    }
    xcb_prefetch_extension_data(connection, &xcb_shape_id);
    xcb_prefetch_extension_data(connection, &xcb_xfixes_id);
    return connection;
}

static void allow_lazy_exit(xcb_connection_t *connection)
{
    const xcb_query_extension_reply_t *xfixes = xcb_get_extension_data(connection, &xcb_xfixes_id);
    if (xfixes == NULL || !xfixes->present) {
        return;
    }
    xcb_xfixes_query_version_cookie_t cookie =
        xcb_xfixes_query_version(connection, XFIXES_DISCONNECT_MODE_MAJOR, 0);
    xcb_xfixes_query_version_reply_t *reply =
        xcb_xfixes_query_version_reply(connection, cookie, NULL);
    if (reply != NULL && reply->major_version >= XFIXES_DISCONNECT_MODE_MAJOR) {
        xcb_xfixes_set_client_disconnect_mode(connection,
                                              XCB_XFIXES_CLIENT_DISCONNECT_FLAGS_TERMINATE);
    }
    free(reply);
}

static void drop_queued_events(xcb_connection_t *connection)
{
    xcb_generic_event_t *event;
    while ((event = xcb_poll_for_queued_event(connection)) != NULL) {
        free(event);
    }
}

xcb_window_t x11_link_root(xcb_connection_t *connection)
{
    return xcb_setup_roots_iterator(xcb_get_setup(connection)).data->root;
}

xcb_atom_t x11_link_atom(const x11_link *link, x11_atom_id id)
{
    return link->atoms.ids[id];
}

void x11_link_set_bytes(const x11_link *link, const x11_property *property, x11_bytes bytes)
{
    xcb_connection_t *connection = x11_link_connection(link);
    if (connection == NULL) {
        return;
    }
    xcb_change_property(connection, XCB_PROP_MODE_REPLACE, property->window,
                        property->atom, property->type, BYTE_FORMAT,
                        bytes.length, bytes.data);
}

void x11_link_delete(const x11_link *link, const x11_property *property)
{
    xcb_connection_t *connection = x11_link_connection(link);
    if (connection != NULL) {
        xcb_delete_property(connection, property->window, property->atom);
    }
}

void x11_link_flush(const x11_link *link)
{
    xcb_connection_t *connection = x11_link_connection(link);
    if (connection != NULL) {
        xcb_flush(connection);
        drop_queued_events(connection);
    }
}

void x11_link_set_cardinals(const x11_link *link, const x11_property *property,
                            x11_values values)
{
    change_property(link, &(property_change){property, XCB_PROP_MODE_REPLACE, values});
}

void x11_link_set_atoms(const x11_link *link, const x11_property *property, x11_atom_list list)
{
    uint32_t atoms[ATOM_COUNT];
    x11_values values = atom_values(link, list, atoms);
    change_property(link, &(property_change){property, XCB_PROP_MODE_REPLACE, values});
}

void x11_link_append_atoms(const x11_link *link, const x11_property *property,
                           x11_atom_list list)
{
    uint32_t atoms[ATOM_COUNT];
    x11_values values = atom_values(link, list, atoms);
    change_property(link, &(property_change){property, XCB_PROP_MODE_APPEND, values});
}

static x11_values atom_values(const x11_link *link, x11_atom_list list, uint32_t *atoms)
{
    uint32_t used = list.count < ATOM_COUNT ? list.count : ATOM_COUNT;
    for (uint32_t i = 0; i < used; i++) {
        atoms[i] = x11_link_atom(link, list.ids[i]);
    }
    return (x11_values){atoms, used};
}

static void change_property(const x11_link *link, const property_change *change)
{
    xcb_connection_t *connection = x11_link_connection(link);
    if (connection == NULL) {
        return;
    }
    const x11_property *property = change->property;
    xcb_change_property(connection, change->mode, property->window,
                        property->atom, property->type, CARDINAL_FORMAT,
                        change->values.count, change->values.data);
}

void x11_link_read(const x11_link *link, x11_read *reads, size_t count)
{
    xcb_connection_t *connection = x11_link_connection(link);
    xcb_get_property_cookie_t cookies[X11_READS_MAX];
    count = count < X11_READS_MAX ? count : X11_READS_MAX;
    for (size_t i = 0; i < count; i++) {
        reads[i].count = 0;
        if (connection != NULL) {
            cookies[i] = request_read(connection, &reads[i]);
        }
    }
    for (size_t i = 0; connection != NULL && i < count; i++) {
        receive_read(connection, cookies[i], &reads[i]);
    }
    x11_link_flush(link);
}

static xcb_get_property_cookie_t request_read(xcb_connection_t *connection,
                                              const x11_read *read)
{
    uint32_t words = read->kind == X11_READ_TEXT ? (read->size + 3) / 4 : read->size;
    const x11_property *p = &read->property;
    return xcb_get_property(connection, 0, p->window, p->atom, p->type, 0, words);
}

static void receive_read(xcb_connection_t *connection, xcb_get_property_cookie_t cookie,
                         x11_read *read)
{
    xcb_generic_error_t *error = NULL;
    xcb_get_property_reply_t *reply = xcb_get_property_reply(connection, cookie, &error);
    free(error);
    if (reply != NULL) {
        store_read(read, reply);
        free(reply);
    }
}

x11_shape_query x11_link_request_shape(const x11_link *link, xcb_window_t window)
{
    x11_shape_query query = {false, {0}};
    xcb_connection_t *connection = x11_link_connection(link);
    if (connection == NULL) {
        return query;
    }
    const xcb_query_extension_reply_t *shape = xcb_get_extension_data(connection, &xcb_shape_id);
    if (shape != NULL && shape->present) {
        query = (x11_shape_query){true, xcb_shape_query_extents(connection, window)};
    }
    return query;
}

bool x11_link_collect_shape(const x11_link *link, const x11_shape_query *query)
{
    xcb_connection_t *connection = x11_link_connection(link);
    if (!query->sent || connection == NULL) {
        return false;
    }
    xcb_generic_error_t *error = NULL;
    xcb_shape_query_extents_reply_t *reply =
        xcb_shape_query_extents_reply(connection, query->cookie, &error);
    free(error);
    bool shaped = reply != NULL && reply->bounding_shaped;
    free(reply);
    x11_link_flush(link);
    return shaped;
}

static void store_read(x11_read *read, xcb_get_property_reply_t *reply)
{
    uint32_t length = (uint32_t) xcb_get_property_value_length(reply);
    if (read->kind == X11_READ_TEXT && reply->format == BYTE_FORMAT && read->size > 0) {
        read->count = length < read->size - 1 ? length : read->size - 1;
        memcpy(read->data, xcb_get_property_value(reply), read->count);
        ((char *) read->data)[read->count] = '\0';
    } else if (read->kind == X11_READ_CARDINALS && reply->format == CARDINAL_FORMAT) {
        uint32_t values = length / sizeof(uint32_t);
        read->count = values < read->size ? values : read->size;
        memcpy(read->data, xcb_get_property_value(reply), read->count * sizeof(uint32_t));
    }
}
