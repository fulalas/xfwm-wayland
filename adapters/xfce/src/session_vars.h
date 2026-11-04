#ifndef XFCE_SESSION_VARS_H
#define XFCE_SESSION_VARS_H

typedef struct session_variable {
    const char *name;
    const char *value;
} session_variable;

typedef struct session_lookup {
    void *self;
    const char *(*get)(void *self, const char *name);
} session_lookup;

extern const session_variable session_vars_defaults[];

char **session_vars_activation_argv(const char *tool, const session_lookup *lookup);

#endif
