#ifndef COMPOSITOR_ZOOM_MATH_H
#define COMPOSITOR_ZOOM_MATH_H

#include "wm/geometry.h"

typedef struct zoom_box {
    double x;
    double y;
    double width;
    double height;
} zoom_box;

typedef struct zoom_request {
    wm_size size;
    wm_point center;
    double level;
} zoom_request;

zoom_box zoom_source_box(const zoom_request *request);

#endif
