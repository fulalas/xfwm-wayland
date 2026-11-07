#ifndef COMPOSITOR_LOCK_H
#define COMPOSITOR_LOCK_H

#include <stdbool.h>

#include "server.h"

struct wlr_surface;

session_lock *session_lock_create(compositor *server);
void session_lock_destroy(session_lock *lock);
bool session_lock_is_locked(const session_lock *lock);
struct wlr_surface *session_lock_focus(const session_lock *lock);
void session_lock_arrange(session_lock *lock);

#endif
