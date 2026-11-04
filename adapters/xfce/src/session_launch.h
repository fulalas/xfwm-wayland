#ifndef XFCE_SESSION_LAUNCH_H
#define XFCE_SESSION_LAUNCH_H

typedef struct session_launch_listener {
    void *self;
    void (*ended)(void *self);
} session_launch_listener;

typedef struct session_launch session_launch;

session_launch *session_launch_start(const char *command, const session_launch_listener *listener);
void session_launch_destroy(session_launch *launch);
int session_launch_exit_code(const session_launch *launch);

#endif
