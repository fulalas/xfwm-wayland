#include "trim_rule.h"

#include <stdbool.h>

static trim_plan release(int backoff_ms);
static trim_plan wait_or_rest(const trim_state *state, const trim_tick *tick, int64_t since);
static trim_plan wait(int backoff_ms, int64_t wait_ms);
static int doubled(int backoff_ms);

trim_state trim_rule_start(int64_t now_ms)
{
    return (trim_state) {
        .released_ms = now_ms - TRIM_ROUTINE_MS,
        .backoff_ms = TRIM_BACKOFF_MIN_MS,
    };
}

trim_plan trim_rule_plan(const trim_state *state, const trim_tick *tick)
{
    int64_t since = tick->now_ms - state->released_ms;
    bool calm = tick->frames <= TRIM_CALM_FRAMES_MAX;
    bool waited = since >= state->backoff_ms;
    bool came_back = tick->bytes >= state->bytes_after + TRIM_GROWTH_BYTES;
    if (tick->bytes >= state->bytes_before + TRIM_GROWTH_BYTES) {
        return release(TRIM_BACKOFF_MIN_MS);
    }
    if (waited && came_back) {
        return release(doubled(state->backoff_ms));
    }
    if (calm && since >= TRIM_ROUTINE_MS &&
        tick->bytes >= state->bytes_after + TRIM_LEFTOVER_BYTES) {
        return release(TRIM_BACKOFF_MIN_MS);
    }
    if (!waited && tick->bytes >= state->bytes_checked + TRIM_GROWTH_BYTES) {
        return (trim_plan) {.step = TRIM_CHECK_NEW, .backoff_ms = state->backoff_ms};
    }
    return wait_or_rest(state, tick, since);
}

static trim_plan release(int backoff_ms)
{
    return (trim_plan) {.step = TRIM_RELEASE, .backoff_ms = backoff_ms};
}

static trim_plan wait_or_rest(const trim_state *state, const trim_tick *tick, int64_t since)
{
    int backoff_ms = since >= state->backoff_ms ? TRIM_BACKOFF_MIN_MS : state->backoff_ms;
    if (tick->frames > TRIM_CALM_FRAMES_MAX) {
        return wait(backoff_ms, TRIM_TICK_MS);
    }
    if (tick->bytes >= state->bytes_after + TRIM_GROWTH_BYTES) {
        return wait(backoff_ms, backoff_ms - since);
    }
    if (tick->bytes >= state->bytes_after + TRIM_LEFTOVER_BYTES) {
        return wait(backoff_ms, TRIM_ROUTINE_MS - since);
    }
    return (trim_plan) {.step = TRIM_REST, .backoff_ms = backoff_ms};
}

static trim_plan wait(int backoff_ms, int64_t wait_ms)
{
    return (trim_plan) {
        .step = TRIM_WAIT,
        .backoff_ms = backoff_ms,
        .wait_ms = wait_ms > TRIM_TICK_MS ? (int) wait_ms : TRIM_TICK_MS,
    };
}

static int doubled(int backoff_ms)
{
    return backoff_ms * 2 < TRIM_BACKOFF_MAX_MS ? backoff_ms * 2 : TRIM_BACKOFF_MAX_MS;
}
