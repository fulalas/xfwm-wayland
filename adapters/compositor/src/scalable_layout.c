#include "scalable_layout.h"

#include <math.h>

static const double TRUNCATION_MARGIN = 0.5;

scalable_layout scalable_layout_for(const scalable_request *request)
{
    double factor = request->cursor_size / request->nominal_size;
    long logical = lround(request->canvas_size * factor);
    if (logical < 1) {
        logical = 1;
    }
    long pixels = lround((double) logical * request->output_scale);
    if (pixels < 1) {
        pixels = 1;
    }
    return (scalable_layout) {
        .pixels = (int) pixels,
        .buffer_scale = (float) ((double) pixels / ((double) logical + TRUNCATION_MARGIN)),
        .hotspot_factor = factor,
    };
}
