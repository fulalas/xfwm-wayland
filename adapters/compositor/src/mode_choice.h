#ifndef COMPOSITOR_MODE_CHOICE_H
#define COMPOSITOR_MODE_CHOICE_H

#include <stdbool.h>
#include <stddef.h>

#include "compositor/displays.h"

typedef struct mode_option {
    display_mode mode;
    bool is_preferred;
} mode_option;

typedef struct mode_options {
    const mode_option *options;
    size_t count;
} mode_options;

enum {
    MODE_CHOICE_NONE = -1,
};

int mode_choice_pick(const mode_options *options, const display_mode *wanted);

#endif
