#include <time.h>

#include "clock.h"

int64_t clock_now_ns(void)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (int64_t) now.tv_sec * CLOCK_NS_PER_SECOND + now.tv_nsec;
}

int64_t clock_now_ms(void)
{
    return clock_now_ns() / CLOCK_NS_PER_MS;
}
