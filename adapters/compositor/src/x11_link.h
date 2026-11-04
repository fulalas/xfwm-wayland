#ifndef COMPOSITOR_X11_LINK_H
#define COMPOSITOR_X11_LINK_H

#include <stdbool.h>
#include <stdint.h>
#include <xcb/xcb.h>

#include "x11_atoms.h"

struct wlr_xwayland;

typedef struct x11_link {
    struct wlr_xwayland *wlr;
    x11_atoms atoms;
    bool ready;
} x11_link;

typedef struct x11_property {
    xcb_window_t window;
    xcb_atom_t atom;
    xcb_atom_t type;
} x11_property;

void x11_link_attach(x11_link *link, struct wlr_xwayland *wlr);
xcb_connection_t *x11_link_connection(const x11_link *link);
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
void x11_link_set_cardinals(const x11_link *link, const x11_property *property,
                            x11_values values);
void x11_link_set_atoms(const x11_link *link, const x11_property *property, x11_atom_list list);
void x11_link_append_atoms(const x11_link *link, const x11_property *property,
                           x11_atom_list list);

#endif
