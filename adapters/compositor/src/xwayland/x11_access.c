#include "x11_access.h"

#include <stdlib.h>

static const char root_user[] = "localuser\0root";

static xcb_void_cookie_t insert_root(xcb_connection_t *connection, bool checked)
{
    return (checked ? xcb_change_hosts_checked : xcb_change_hosts)(
        connection, XCB_HOST_MODE_INSERT, XCB_FAMILY_SERVER_INTERPRETED, sizeof(root_user) - 1,
        (const uint8_t *) root_user);
}

void x11_access_allow_root(xcb_connection_t *connection)
{
    if (connection == NULL) {
        return;
    }
    insert_root(connection, false);
    xcb_flush(connection);
}

bool x11_access_allow_root_on(const char *display)
{
    xcb_connection_t *connection = xcb_connect(display, NULL);
    bool allowed = !xcb_connection_has_error(connection);
    if (allowed) {
        xcb_generic_error_t *error = xcb_request_check(connection, insert_root(connection, true));
        allowed = error == NULL;
        free(error);
    }
    xcb_disconnect(connection);
    return allowed;
}
