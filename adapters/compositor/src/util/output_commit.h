#ifndef COMPOSITOR_OUTPUT_COMMIT_H
#define COMPOSITOR_OUTPUT_COMMIT_H

#include <stdbool.h>
#include <wlr/types/wlr_output.h>

static inline bool output_commit_latched(const struct wlr_output_event_commit *event)
{
    return (event->state->committed & WLR_OUTPUT_STATE_BUFFER) != 0 || !event->output->enabled;
}

#endif
