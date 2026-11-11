#include "x11_scale.h"

#include <math.h>

static const double ROUNDING_SLACK = 1e-9;

static int length_from_x(double scale, int x);

int x11_scale_to_x(double scale, int logical)
{
    return (int) lround(logical * scale);
}

int x11_scale_from_x(double scale, int x)
{
    return (int) lround(x / scale);
}

wm_rect x11_scale_rect_to_x(double scale, wm_rect logical)
{
    return (wm_rect){x11_scale_to_x(scale, logical.x), x11_scale_to_x(scale, logical.y),
                     x11_scale_to_x(scale, logical.width), x11_scale_to_x(scale, logical.height)};
}

wm_rect x11_scale_rect_from_x(double scale, wm_rect x)
{
    return (wm_rect){x11_scale_from_x(scale, x.x), x11_scale_from_x(scale, x.y),
                     length_from_x(scale, x.width), length_from_x(scale, x.height)};
}

wm_size x11_scale_size_from_x(double scale, wm_size x)
{
    return (wm_size){length_from_x(scale, x.width), length_from_x(scale, x.height)};
}

wm_size x11_scale_size_from_x_up(double scale, wm_size x)
{
    return (wm_size){(int) ceil(x.width / scale - ROUNDING_SLACK),
                     (int) ceil(x.height / scale - ROUNDING_SLACK)};
}

wm_size x11_scale_size_from_x_down(double scale, wm_size x)
{
    wm_size size = {(int) floor(x.width / scale + ROUNDING_SLACK),
                    (int) floor(x.height / scale + ROUNDING_SLACK)};
    return (wm_size){x.width > 0 ? wm_max(size.width, 1) : 0,
                     x.height > 0 ? wm_max(size.height, 1) : 0};
}

wm_extents x11_scale_extents_to_x(double scale, wm_extents logical)
{
    return (wm_extents){x11_scale_to_x(scale, logical.left), x11_scale_to_x(scale, logical.right),
                        x11_scale_to_x(scale, logical.top), x11_scale_to_x(scale, logical.bottom)};
}

static int length_from_x(double scale, int x)
{
    int length = x11_scale_from_x(scale, x);
    return x > 0 && length < 1 ? 1 : length;
}
