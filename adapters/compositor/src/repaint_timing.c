#include "repaint_timing.h"

#include <stdlib.h>
#include <string.h>

enum {
    HISTORY_MIN_SAMPLES = 4,
    BUDGET_PERCENTILE = 90,
    DELAY_CAP_PERCENT = 80,
    PERCENT = 100,
    NS_PER_MS = 1000000,
};

static const int64_t BUDGET_MARGIN_NS = 2 * NS_PER_MS;

static int compare_ns(const void *a, const void *b);
static int64_t next_vblank_ns(const vblank_prediction *vblank, int64_t now_ns);

void render_history_add(render_history *h, int64_t duration_ns)
{
    h->samples_ns[h->next] = duration_ns;
    h->next = (h->next + 1) % RENDER_HISTORY_MAX;
    if (h->count < RENDER_HISTORY_MAX) {
        h->count++;
    }
}

int64_t render_history_budget(const render_history *h)
{
    if (h->count < HISTORY_MIN_SAMPLES) {
        return REPAINT_NO_BUDGET;
    }
    int64_t sorted[RENDER_HISTORY_MAX];
    memcpy(sorted, h->samples_ns, h->count * sizeof(sorted[0]));
    qsort(sorted, h->count, sizeof(sorted[0]), compare_ns);
    size_t index = (h->count * BUDGET_PERCENTILE + PERCENT - 1) / PERCENT - 1;
    return sorted[index] + BUDGET_MARGIN_NS;
}

int repaint_delay_ms(const repaint_plan *plan)
{
    int64_t refresh = plan->vblank.refresh_ns;
    if (refresh <= 0 || plan->render_budget_ns < 0) {
        return 0;
    }
    int64_t left = next_vblank_ns(&plan->vblank, plan->now_ns) - plan->now_ns;
    int64_t delay = left - plan->render_budget_ns;
    int64_t cap = refresh * DELAY_CAP_PERCENT / PERCENT;
    if (delay > cap) {
        delay = cap;
    }
    return delay >= NS_PER_MS ? (int) (delay / NS_PER_MS) : 0;
}

static int compare_ns(const void *a, const void *b)
{
    int64_t left = *(const int64_t *) a;
    int64_t right = *(const int64_t *) b;
    return (left > right) - (left < right);
}

static int64_t next_vblank_ns(const vblank_prediction *vblank, int64_t now_ns)
{
    if (now_ns < vblank->last_ns) {
        return vblank->last_ns + vblank->refresh_ns;
    }
    int64_t periods = (now_ns - vblank->last_ns) / vblank->refresh_ns + 1;
    return vblank->last_ns + periods * vblank->refresh_ns;
}
