#ifndef COMPOSITOR_FIND_CURSOR_RINGS_H
#define COMPOSITOR_FIND_CURSOR_RINGS_H

#include <stdbool.h>
#include <stdint.h>

enum {
    FIND_CURSOR_DURATION_MS = 830,
    FIND_CURSOR_MIN_RADIUS = 1,
    FIND_CURSOR_MAX_RADIUS = 250,
    FIND_CURSOR_RING_GAP = 30,
    FIND_CURSOR_RINGS_MAX = 4,
    FIND_CURSOR_LINE_WIDTH = 3,
};

typedef struct find_cursor_frame {
    double radius;
    int rings;
} find_cursor_frame;

bool find_cursor_is_running(uint32_t elapsed_ms);
find_cursor_frame find_cursor_frame_at(uint32_t elapsed_ms);
double find_cursor_ring_radius(const find_cursor_frame *frame, int ring);
int find_cursor_half_size(const find_cursor_frame *frame);

#endif
