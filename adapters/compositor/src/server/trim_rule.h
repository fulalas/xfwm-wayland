#ifndef COMPOSITOR_TRIM_RULE_H
#define COMPOSITOR_TRIM_RULE_H

#include <stddef.h>
#include <stdint.h>

enum {
    TRIM_TICK_MS = 500,
    TRIM_BACKOFF_MIN_MS = 2000,
    TRIM_BACKOFF_MAX_MS = 64000,
    TRIM_ROUTINE_MS = 60000,
    TRIM_CALM_FRAMES_MAX = 2,
    TRIM_GROWTH_BYTES = 16 * 1024 * 1024,
    TRIM_LEFTOVER_BYTES = 1024 * 1024,
};

typedef struct trim_state {
    int64_t released_ms;
    int backoff_ms;
    size_t bytes_before;
    size_t bytes_after;
    size_t bytes_checked;
} trim_state;

typedef struct trim_tick {
    int64_t now_ms;
    size_t bytes;
    unsigned frames;
} trim_tick;

typedef enum trim_step {
    TRIM_RELEASE,
    TRIM_CHECK_NEW,
    TRIM_WAIT,
    TRIM_REST,
} trim_step;

typedef struct trim_plan {
    trim_step step;
    int backoff_ms;
    int wait_ms;
} trim_plan;

trim_state trim_rule_start(int64_t now_ms);
trim_plan trim_rule_plan(const trim_state *state, const trim_tick *tick);

#endif
