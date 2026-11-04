#include "present_rules.h"

enum {
    CHECK_MARGIN_NS = 500000,
    MARGIN_FRACTION = 4,
};

static int64_t check_margin(const vblank_clock *clock);

bool fifo_must_wait(const fifo_counters *counters)
{
    return counters->barrier || counters->applied != counters->committed;
}

bool fifo_waiter_ready(const fifo_counters *counters, uint32_t needed)
{
    return !counters->barrier && (int32_t) (counters->applied - needed) >= 0;
}

int64_t vblank_clock_next(const vblank_clock *clock, int64_t time_ns)
{
    if (time_ns < clock->last_ns) {
        return clock->last_ns;
    }
    int64_t periods = (time_ns - clock->last_ns) / clock->refresh_ns + 1;
    return clock->last_ns + periods * clock->refresh_ns;
}

int64_t timing_earliest_present(const vblank_clock *clock, output_moment moment)
{
    if (!clock->predictable) {
        return moment.now_ns;
    }
    int64_t next = vblank_clock_next(clock, moment.now_ns);
    return moment.frame_pending ? next + clock->refresh_ns : next;
}

int64_t timing_next_check(const vblank_clock *clock, int64_t target_ns, int64_t now_ns)
{
    if (!clock->predictable) {
        return target_ns;
    }
    int64_t target_vblank = vblank_clock_next(clock, target_ns - 1);
    int64_t candidate = target_vblank - 2 * clock->refresh_ns + check_margin(clock);
    if (candidate > now_ns) {
        return candidate;
    }
    return vblank_clock_next(clock, now_ns) + check_margin(clock);
}

static int64_t check_margin(const vblank_clock *clock)
{
    int64_t fraction = clock->refresh_ns / MARGIN_FRACTION;
    return fraction < CHECK_MARGIN_NS ? fraction : CHECK_MARGIN_NS;
}
