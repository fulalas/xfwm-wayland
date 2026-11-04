#include "find_cursor_rings.h"

#include <math.h>

static double radius_at(uint32_t elapsed_ms);
static int rings_for(double radius);

bool find_cursor_is_running(uint32_t elapsed_ms)
{
    return elapsed_ms < FIND_CURSOR_DURATION_MS;
}

find_cursor_frame find_cursor_frame_at(uint32_t elapsed_ms)
{
    double radius = radius_at(elapsed_ms);
    return (find_cursor_frame){radius, rings_for(radius)};
}

double find_cursor_ring_radius(const find_cursor_frame *frame, int ring)
{
    return frame->radius - ring * FIND_CURSOR_RING_GAP;
}

int find_cursor_half_size(const find_cursor_frame *frame)
{
    return (int) ceil(frame->radius + FIND_CURSOR_LINE_WIDTH / 2.0);
}

static double radius_at(uint32_t elapsed_ms)
{
    double progress = find_cursor_is_running(elapsed_ms)
                          ? (double) elapsed_ms / FIND_CURSOR_DURATION_MS
                          : 1.0;
    return FIND_CURSOR_MIN_RADIUS + progress * (FIND_CURSOR_MAX_RADIUS - FIND_CURSOR_MIN_RADIUS);
}

static int rings_for(double radius)
{
    int rings = (int) ceil(radius / FIND_CURSOR_RING_GAP);
    if (rings < 1) {
        return 1;
    }
    return rings > FIND_CURSOR_RINGS_MAX ? FIND_CURSOR_RINGS_MAX : rings;
}
