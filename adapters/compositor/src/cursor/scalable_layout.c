#include "scalable_layout.h"

#include <math.h>

static const double TRUNCATION_MARGIN = 0.5;
static const double LENGTH_MAX = 1024.0;

scalable_layout scalable_layout_for(const scalable_request *request)
{
    double factor = request->cursor_size / request->nominal_size;
    double length = request->canvas_size * factor;
    if (length > LENGTH_MAX) {
        factor *= LENGTH_MAX / length;
    }
    long logical = lround(fmin(length, LENGTH_MAX));
    if (logical < 1) {
        logical = 1;
    }
    long pixels = lround(fmin((double) logical * request->output_scale, LENGTH_MAX));
    if (pixels < 1) {
        pixels = 1;
    }
    return (scalable_layout) {
        .pixels = (int) pixels,
        .buffer_scale = (float) ((double) pixels / ((double) logical + TRUNCATION_MARGIN)),
        .hotspot_factor = factor,
    };
}
