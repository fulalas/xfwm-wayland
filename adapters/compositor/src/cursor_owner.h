#ifndef COMPOSITOR_CURSOR_OWNER_H
#define COMPOSITOR_CURSOR_OWNER_H

#include <stdbool.h>

typedef struct cursor_request {
    const void *requester;
    const void *pointer_focus;
    const void *drag_source;
    bool drag_active;
    bool client_mode;
} cursor_request;

bool cursor_owner_accepts(const cursor_request *request);

#endif
