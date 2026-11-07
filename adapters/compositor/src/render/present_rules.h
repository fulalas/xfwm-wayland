#ifndef COMPOSITOR_PRESENT_RULES_H
#define COMPOSITOR_PRESENT_RULES_H

#include <stdbool.h>
#include <stdint.h>

typedef struct fifo_counters {
    bool barrier;
    uint32_t committed;
    uint32_t applied;
} fifo_counters;

typedef struct vblank_clock {
    int64_t last_ns;
    int64_t refresh_ns;
    bool predictable;
} vblank_clock;

typedef struct output_moment {
    int64_t now_ns;
    bool frame_pending;
} output_moment;

bool fifo_must_wait(const fifo_counters *counters);
bool fifo_waiter_ready(const fifo_counters *counters, uint32_t needed);

int64_t vblank_clock_next(const vblank_clock *clock, int64_t time_ns);
int64_t vblank_clock_next_frame(const vblank_clock *clock, int64_t now_ns);
int64_t timing_earliest_present(const vblank_clock *clock, output_moment moment);
int64_t timing_next_check(const vblank_clock *clock, int64_t target_ns, int64_t now_ns);

#endif
