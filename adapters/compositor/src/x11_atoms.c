#include "x11_atoms.h"

#include <stdlib.h>
#include <string.h>
#include <wlr/util/log.h>

static const char *const atom_names[ATOM_COUNT] = {
    [ATOM_NET_SUPPORTED] = "_NET_SUPPORTED",
    [ATOM_NET_NUMBER_OF_DESKTOPS] = "_NET_NUMBER_OF_DESKTOPS",
    [ATOM_NET_CURRENT_DESKTOP] = "_NET_CURRENT_DESKTOP",
    [ATOM_NET_DESKTOP_NAMES] = "_NET_DESKTOP_NAMES",
    [ATOM_NET_DESKTOP_GEOMETRY] = "_NET_DESKTOP_GEOMETRY",
    [ATOM_NET_DESKTOP_VIEWPORT] = "_NET_DESKTOP_VIEWPORT",
    [ATOM_NET_WORKAREA] = "_NET_WORKAREA",
    [ATOM_GTK_WORKAREAS] = "_GTK_WORKAREAS",
    [ATOM_NET_WM_DESKTOP] = "_NET_WM_DESKTOP",
    [ATOM_NET_FRAME_EXTENTS] = "_NET_FRAME_EXTENTS",
    [ATOM_NET_REQUEST_FRAME_EXTENTS] = "_NET_REQUEST_FRAME_EXTENTS",
    [ATOM_NET_MOVERESIZE_WINDOW] = "_NET_MOVERESIZE_WINDOW",
    [ATOM_GTK_SHOW_WINDOW_MENU] = "_GTK_SHOW_WINDOW_MENU",
    [ATOM_NET_WM_ALLOWED_ACTIONS] = "_NET_WM_ALLOWED_ACTIONS",
    [ATOM_NET_WM_ACTION_MOVE] = "_NET_WM_ACTION_MOVE",
    [ATOM_NET_WM_ACTION_RESIZE] = "_NET_WM_ACTION_RESIZE",
    [ATOM_NET_WM_ACTION_MINIMIZE] = "_NET_WM_ACTION_MINIMIZE",
    [ATOM_NET_WM_ACTION_SHADE] = "_NET_WM_ACTION_SHADE",
    [ATOM_NET_WM_ACTION_STICK] = "_NET_WM_ACTION_STICK",
    [ATOM_NET_WM_ACTION_MAXIMIZE_HORZ] = "_NET_WM_ACTION_MAXIMIZE_HORZ",
    [ATOM_NET_WM_ACTION_MAXIMIZE_VERT] = "_NET_WM_ACTION_MAXIMIZE_VERT",
    [ATOM_NET_WM_ACTION_FULLSCREEN] = "_NET_WM_ACTION_FULLSCREEN",
    [ATOM_NET_WM_ACTION_CHANGE_DESKTOP] = "_NET_WM_ACTION_CHANGE_DESKTOP",
    [ATOM_NET_WM_ACTION_CLOSE] = "_NET_WM_ACTION_CLOSE",
    [ATOM_NET_WM_ACTION_ABOVE] = "_NET_WM_ACTION_ABOVE",
    [ATOM_NET_WM_ACTION_BELOW] = "_NET_WM_ACTION_BELOW",
    [ATOM_UTF8_STRING] = "UTF8_STRING",
    [ATOM_XSETTINGS_SELECTION] = "_XSETTINGS_S0",
    [ATOM_XSETTINGS_SETTINGS] = "_XSETTINGS_SETTINGS",
    [ATOM_MANAGER] = "MANAGER",
};

bool x11_atoms_intern(x11_atoms *atoms, xcb_connection_t *connection)
{
    xcb_intern_atom_cookie_t cookies[ATOM_COUNT];
    for (size_t i = 0; i < ATOM_COUNT; i++) {
        cookies[i] = xcb_intern_atom(connection, 0, (uint16_t) strlen(atom_names[i]),
                                     atom_names[i]);
    }
    bool complete = true;
    for (size_t i = 0; i < ATOM_COUNT; i++) {
        xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(connection, cookies[i], NULL);
        atoms->ids[i] = reply != NULL ? reply->atom : XCB_ATOM_NONE;
        complete = complete && reply != NULL;
        free(reply);
    }
    if (!complete) {
        wlr_log(WLR_ERROR, "cannot intern the X11 atoms for EWMH and XSETTINGS");
    }
    return complete;
}
