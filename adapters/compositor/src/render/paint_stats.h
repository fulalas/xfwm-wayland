#ifndef COMPOSITOR_PAINT_STATS_H
#define COMPOSITOR_PAINT_STATS_H

#include <stdbool.h>
#include <stdint.h>

typedef struct paint_stats {
    bool enabled;
    int64_t since_ns;
    unsigned painted;
} paint_stats;

paint_stats paint_stats_init(void);
void paint_stats_note(paint_stats *stats, const char *output_name);

#endif
