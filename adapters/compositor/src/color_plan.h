#ifndef COMPOSITOR_COLOR_PLAN_H
#define COMPOSITOR_COLOR_PLAN_H

#include <stdbool.h>
#include <stddef.h>

enum {
    COLOR_ATTEMPTS_MAX = 3,
};

typedef struct color_mode {
    bool hdr;
    bool ten_bit;
} color_mode;

typedef struct color_plan {
    color_mode attempts[COLOR_ATTEMPTS_MAX];
    size_t count;
} color_plan;

color_plan color_plan_for(color_mode wanted);

#endif
