#ifndef COMPOSITOR_DISPLAY_POLICY_H
#define COMPOSITOR_DISPLAY_POLICY_H

#include <stdbool.h>
#include <stddef.h>

typedef struct display_wish {
    bool is_internal;
    bool wants_enabled;
} display_wish;

typedef struct display_wishes {
    const display_wish *wishes;
    size_t count;
    bool lid_closed;
} display_wishes;

bool display_is_internal(const char *connector);
void display_policy_decide(const display_wishes *wishes, bool *enabled);

#endif
