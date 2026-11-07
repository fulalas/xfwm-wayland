#include "restart_policy.h"

enum {
    CRASH_WINDOW_MS = 3 * 60 * 1000,
    MAX_CRASHES = 5,
    FIRST_DELAY_MS = 1000,
};

void restart_policy_record_crash(restart_policy *policy, int64_t now_ms)
{
    if (policy->crashes == 0 || now_ms - policy->first_crash_ms > CRASH_WINDOW_MS) {
        policy->first_crash_ms = now_ms;
        policy->crashes = 0;
    }
    policy->crashes++;
}

restart_decision restart_policy_next(const restart_policy *policy)
{
    if (policy->crashes == 0 || policy->crashes >= MAX_CRASHES) {
        return (restart_decision){policy->crashes == 0, 0};
    }
    return (restart_decision){true, (uint32_t) FIRST_DELAY_MS << (policy->crashes - 1)};
}
