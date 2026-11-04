#include "x11_link.h"

#include <wlr/xwayland/xwayland.h>

enum {
    BYTE_FORMAT = 8,
    CARDINAL_FORMAT = 32,
};

typedef struct property_change {
    const x11_property *property;
    uint8_t mode;
    x11_values values;
} property_change;

static x11_values atom_values(const x11_link *link, x11_atom_list list, uint32_t *atoms);
static void change_property(const x11_link *link, const property_change *change);

void x11_link_attach(x11_link *link, struct wlr_xwayland *wlr)
{
    link->wlr = wlr;
    xcb_connection_t *connection = wlr_xwayland_get_xwm_connection(wlr);
    link->ready = connection != NULL && x11_atoms_intern(&link->atoms, connection);
}

xcb_connection_t *x11_link_connection(const x11_link *link)
{
    if (!link->ready || link->wlr == NULL) {
        return NULL;
    }
    return wlr_xwayland_get_xwm_connection(link->wlr);
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
    xcb_flush(connection);
}

void x11_link_delete(const x11_link *link, const x11_property *property)
{
    xcb_connection_t *connection = x11_link_connection(link);
    if (connection != NULL) {
        xcb_delete_property(connection, property->window, property->atom);
        xcb_flush(connection);
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
    xcb_flush(connection);
}
