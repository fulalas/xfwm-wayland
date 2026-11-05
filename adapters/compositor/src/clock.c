#include <time.h>

#include "clock.h"

int64_t clock_timespec_ns(const struct timespec *time)
{
    return (int64_t) time->tv_sec * CLOCK_NS_PER_SECOND + time->tv_nsec;
}

int64_t clock_now_ns(void)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return clock_timespec_ns(&now);
}

int64_t clock_now_ms(void)
{
    return clock_now_ns() / CLOCK_NS_PER_MS;
}
