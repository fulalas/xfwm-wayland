#ifndef XFCE_HELPER_SESSION_H
#define XFCE_HELPER_SESSION_H

#include <stdbool.h>

typedef struct session_events {
    void *self;
    void (*line)(void *self, const char *line);
    void (*exited)(void *self);
} session_events;

typedef struct helper_session helper_session;

helper_session *helper_session_spawn(const char *path, const session_events *events);
void helper_session_destroy(helper_session *session);
void helper_session_send(helper_session *session, const char *line);

#endif
