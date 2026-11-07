#include "color_plan.h"

static void add_attempt(color_plan *plan, color_mode mode);

color_plan color_plan_for(color_mode wanted)
{
    color_plan plan = {.count = 0};
    if (wanted.hdr) {
        add_attempt(&plan, (color_mode) {.hdr = true, .ten_bit = true});
    }
    if (wanted.ten_bit) {
        add_attempt(&plan, (color_mode) {.hdr = false, .ten_bit = true});
    }
    add_attempt(&plan, (color_mode) {.hdr = false, .ten_bit = false});
    return plan;
}

static void add_attempt(color_plan *plan, color_mode mode)
{
    plan->attempts[plan->count++] = mode;
}
