#include "repaint_timing.h"

#include <stdlib.h>
#include <string.h>

#include "util/clock.h"

enum {
    HISTORY_MIN_SAMPLES = 4,
    BUDGET_PERCENTILE = 90,
    DELAY_CAP_PERCENT = 80,
    PERCENT = 100,
};

static const int64_t BUDGET_MARGIN_NS = 2 * CLOCK_NS_PER_MS;

static size_t sample_count(const render_history *history);
static int compare_ns(const void *a, const void *b);

void render_history_add(render_history *h, int64_t duration_ns)
{
    h->samples_ns[h->total % RENDER_HISTORY_MAX] = duration_ns;
    h->total++;
}

int64_t render_history_budget(const render_history *h)
{
    size_t count = sample_count(h);
    if (count < HISTORY_MIN_SAMPLES) {
        return REPAINT_NO_BUDGET;
    }
    int64_t sorted[RENDER_HISTORY_MAX];
    memcpy(sorted, h->samples_ns, count * sizeof(sorted[0]));
    qsort(sorted, count, sizeof(sorted[0]), compare_ns);
    size_t index = (count * BUDGET_PERCENTILE + PERCENT - 1) / PERCENT - 1;
    return sorted[index] + BUDGET_MARGIN_NS;
}

int repaint_delay_ms(const repaint_plan *plan)
{
    const vblank_clock *vblank = &plan->vblank;
    if (!vblank->predictable || plan->render_budget_ns < 0) {
        return 0;
    }
    int64_t left = vblank_clock_next_frame(vblank, plan->now_ns) - plan->now_ns;
    int64_t delay = left - plan->render_budget_ns;
    int64_t cap = vblank->refresh_ns * DELAY_CAP_PERCENT / PERCENT;
    if (delay > cap) {
        delay = cap;
    }
    return delay >= CLOCK_NS_PER_MS ? (int) (delay / CLOCK_NS_PER_MS) : 0;
}

static size_t sample_count(const render_history *h)
{
    return h->total < RENDER_HISTORY_MAX ? h->total : RENDER_HISTORY_MAX;
}

static int compare_ns(const void *a, const void *b)
{
    int64_t left = *(const int64_t *) a;
    int64_t right = *(const int64_t *) b;
    return (left > right) - (left < right);
}
