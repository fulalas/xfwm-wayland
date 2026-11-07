#ifndef COMPOSITOR_X11_ATOMS_H
#define COMPOSITOR_X11_ATOMS_H

#include <stdbool.h>
#include <xcb/xcb.h>

#include "x11_atom_ids.h"

typedef struct x11_atoms {
    xcb_atom_t ids[ATOM_COUNT];
} x11_atoms;

bool x11_atoms_intern(x11_atoms *atoms, xcb_connection_t *connection);

#endif
