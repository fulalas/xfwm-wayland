#ifndef COMPOSITOR_RESTART_POLICY_H
#define COMPOSITOR_RESTART_POLICY_H

#include <stdbool.h>
#include <stdint.h>

typedef struct restart_policy {
    int64_t first_crash_ms;
    unsigned crashes;
} restart_policy;

typedef struct restart_decision {
    bool restart;
    uint32_t delay_ms;
} restart_decision;

void restart_policy_record_crash(restart_policy *policy, int64_t now_ms);
restart_decision restart_policy_next(const restart_policy *policy);

#endif
