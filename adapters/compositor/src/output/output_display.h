#ifndef COMPOSITOR_OUTPUT_DISPLAY_H
#define COMPOSITOR_OUTPUT_DISPLAY_H

#include <stdbool.h>

#include "compositor/displays.h"
#include "edid.h"

typedef struct output_display {
    edid_blob edid;
    bool is_internal;
    bool is_pending;
    bool wants_enabled;
    bool is_primary;
    bool has_last_enabled;
    display_setting last_enabled;
} output_display;

#endif
