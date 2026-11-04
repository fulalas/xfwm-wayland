#include "zoom_math.h"

static double clamp_double(double value, double low, double high);

zoom_box zoom_source_box(const zoom_request *request)
{
    double level = request->level < 1.0 ? 1.0 : request->level;
    double width = request->size.width;
    double height = request->size.height;
    double cx = clamp_double(request->center.x, 0.0, width);
    double cy = clamp_double(request->center.y, 0.0, height);
    double keep = 1.0 - 1.0 / level;
    zoom_box box = {
        .x = cx * keep,
        .y = cy * keep,
        .width = width / level,
        .height = height / level,
    };
    return box;
}

static double clamp_double(double value, double low, double high)
{
    if (value < low) {
        return low;
    }
    return value > high ? high : value;
}
