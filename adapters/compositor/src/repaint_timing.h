#ifndef COMPOSITOR_REPAINT_TIMING_H
#define COMPOSITOR_REPAINT_TIMING_H

#include <stddef.h>
#include <stdint.h>

#include "present_rules.h"

enum {
    RENDER_HISTORY_MAX = 16,
    REPAINT_NO_BUDGET = -1,
};

typedef struct render_history {
    int64_t samples_ns[RENDER_HISTORY_MAX];
    size_t total;
} render_history;

typedef struct repaint_plan {
    int64_t now_ns;
    vblank_clock vblank;
    int64_t render_budget_ns;
} repaint_plan;

void render_history_add(render_history *history, int64_t duration_ns);
int64_t render_history_budget(const render_history *history);
int repaint_delay_ms(const repaint_plan *plan);

#endif
