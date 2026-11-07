#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util/clock.h"
#include "paint_stats.h"

static const int64_t REPORT_PERIOD_NS = 5 * 1000000000LL;

paint_stats paint_stats_init(void)
{
    const char *value = getenv("XFWM4_PAINT_STATS");
    paint_stats stats = {
        .enabled = value != NULL && strcmp(value, "1") == 0,
        .since_ns = 0,
        .painted = 0,
    };
    return stats;
}

void paint_stats_note(paint_stats *stats, const char *output_name)
{
    if (!stats->enabled) {
        return;
    }
    int64_t now = clock_now_ns();
    if (stats->since_ns == 0) {
        stats->since_ns = now;
    }
    stats->painted++;
    int64_t elapsed = now - stats->since_ns;
    if (elapsed >= REPORT_PERIOD_NS) {
        double seconds = (double) elapsed / (double) CLOCK_NS_PER_SECOND;
        fprintf(stderr, "output %s: paints %.1f/s\n", output_name, stats->painted / seconds);
        stats->painted = 0;
        stats->since_ns = now;
    }
}
