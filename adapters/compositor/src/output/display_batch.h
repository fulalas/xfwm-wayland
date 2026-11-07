#ifndef COMPOSITOR_DISPLAY_BATCH_H
#define COMPOSITOR_DISPLAY_BATCH_H

#include <stdbool.h>
#include <stddef.h>

#include "compositor/displays.h"
#include "display_policy.h"
#include "output.h"

typedef struct display_batch {
    output *outputs[DISPLAY_MAX];
    display_state states[DISPLAY_MAX];
    size_t count;
} display_batch;

void display_batch_collect(const outputs *outputs, display_batch *batch);
bool display_batch_has_pending(const display_batch *batch);
size_t display_batch_wishes(const display_batch *batch, display_wish *wishes);

#endif
