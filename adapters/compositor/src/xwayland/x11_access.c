#include "x11_access.h"

#include <stdlib.h>

static const char root_user[] = "localuser\0root";

void x11_access_allow_root(xcb_connection_t *connection)
{
    if (connection == NULL) {
        return;
    }
    xcb_change_hosts(connection, XCB_HOST_MODE_INSERT, XCB_FAMILY_SERVER_INTERPRETED,
                     sizeof(root_user) - 1, (const uint8_t *) root_user);
    xcb_flush(connection);
}

bool x11_access_allow_root_on(const char *display)
{
    xcb_connection_t *connection = xcb_connect(display, NULL);
    bool connected = !xcb_connection_has_error(connection);
    if (connected) {
        x11_access_allow_root(connection);
        free(xcb_get_input_focus_reply(connection, xcb_get_input_focus(connection), NULL));
    }
    xcb_disconnect(connection);
    return connected;
}
