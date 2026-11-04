#include "logout_policy.h"

#include <string.h>

static const char *const unavailable_errors[] = {
    "org.freedesktop.DBus.Error.ServiceUnknown",
    "org.freedesktop.DBus.Error.NameHasNoOwner",
    "org.freedesktop.DBus.Error.UnknownMethod",
    "org.freedesktop.DBus.Error.UnknownObject",
    "org.freedesktop.DBus.Error.UnknownInterface",
    NULL,
};

bool logout_manager_unavailable(const char *remote_error)
{
    if (!remote_error) {
        return false;
    }
    for (size_t i = 0; unavailable_errors[i]; i++) {
        if (strcmp(remote_error, unavailable_errors[i]) == 0) {
            return true;
        }
    }
    return false;
}
