#ifndef COMPOSITOR_CLOCK_H
#define COMPOSITOR_CLOCK_H

#include <stdint.h>

enum {
    CLOCK_MS_PER_SECOND = 1000,
    CLOCK_NS_PER_MS = 1000000,
    CLOCK_NS_PER_SECOND = 1000000000,
};

int64_t clock_now_ns(void);
int64_t clock_now_ms(void);

#endif
