#ifndef COMPOSITOR_X11_LINK_H
#define COMPOSITOR_X11_LINK_H

#include <stdbool.h>
#include <stdint.h>
#include <xcb/shape.h>
#include <xcb/xcb.h>

#include "x11_atoms.h"

struct wlr_xwayland;

typedef struct x11_link {
    struct wlr_xwayland *wlr;
    xcb_connection_t *queries;
    x11_atoms atoms;
    bool ready;
} x11_link;

typedef struct x11_property {
    xcb_window_t window;
    xcb_atom_t atom;
    xcb_atom_t type;
} x11_property;

void x11_link_attach(x11_link *link, struct wlr_xwayland *wlr);
void x11_link_detach(x11_link *link);
xcb_connection_t *x11_link_connection(const x11_link *link);
xcb_connection_t *x11_link_query_connection(x11_link *link);
void x11_link_release_queries(x11_link *link);
xcb_window_t x11_link_root(xcb_connection_t *connection);
xcb_atom_t x11_link_atom(const x11_link *link, x11_atom_id id);
typedef struct x11_values {
    const uint32_t *data;
    uint32_t count;
} x11_values;

typedef struct x11_atom_list {
    const x11_atom_id *ids;
    uint32_t count;
} x11_atom_list;

typedef struct x11_bytes {
    const char *data;
    uint32_t length;
} x11_bytes;

void x11_link_set_bytes(const x11_link *link, const x11_property *property, x11_bytes bytes);
void x11_link_delete(const x11_link *link, const x11_property *property);
void x11_link_flush(const x11_link *link);
typedef enum x11_read_kind {
    X11_READ_CARDINALS,
    X11_READ_TEXT,
} x11_read_kind;

typedef struct x11_read {
    x11_property property;
    x11_read_kind kind;
    void *data;
    uint32_t size;
    uint32_t count;
} x11_read;

void x11_link_read(x11_link *link, x11_read *reads, size_t count);
typedef struct x11_shape_query {
    xcb_connection_t *connection;
    xcb_shape_query_extents_cookie_t cookie;
} x11_shape_query;

x11_shape_query x11_link_request_shape(x11_link *link, xcb_window_t window);
bool x11_link_collect_shape(x11_link *link, const x11_shape_query *query);
void x11_link_set_cardinals(const x11_link *link, const x11_property *property,
                            x11_values values);
void x11_link_set_atoms(const x11_link *link, const x11_property *property, x11_atom_list list);
void x11_link_append_atoms(const x11_link *link, const x11_property *property,
                           x11_atom_list list);

#endif
