#ifndef XFCE_SHORTCUT_DEFAULTS_H
#define XFCE_SHORTCUT_DEFAULTS_H

#include <stddef.h>

typedef struct shortcut_default {
    const char *accelerator;
    const char *action;
} shortcut_default;

const shortcut_default *shortcut_defaults_table(size_t *count);

#endif
