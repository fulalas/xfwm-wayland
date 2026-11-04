#include <stddef.h>

#include "cursor_owner.h"

bool cursor_owner_accepts(const cursor_request *request)
{
    if (request->requester == NULL) {
        return false;
    }
    if (request->drag_active) {
        return request->requester == request->drag_source;
    }
    return request->client_mode && request->requester == request->pointer_focus;
}
