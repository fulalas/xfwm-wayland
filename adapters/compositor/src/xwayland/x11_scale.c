#include "x11_scale.h"

#include <math.h>

static const double ROUNDING_SLACK = 1e-9;

static int length_from_x(double scale, int x);
static int placed_length(double scale, int start, int length);
static int visible(int x, int length);
static int near_or(double scale, int logical, int hint, int fallback);

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

wm_rect x11_scale_rect_to_x_near(double scale, wm_rect logical, wm_rect hint)
{
    wm_rect x = x11_scale_rect_to_x(scale, logical);
    return (wm_rect){near_or(scale, logical.x, hint.x, x.x), near_or(scale, logical.y, hint.y, x.y),
                     near_or(scale, logical.width, hint.width, x.width),
                     near_or(scale, logical.height, hint.height, x.height)};
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

wm_size x11_scale_placed_size_from_x(double scale, wm_rect x)
{
    return (wm_size){placed_length(scale, x.x, x.width), placed_length(scale, x.y, x.height)};
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
    return (wm_size){visible(x.width, size.width), visible(x.height, size.height)};
}

wm_extents x11_scale_extents_to_x(double scale, wm_extents logical)
{
    return (wm_extents){x11_scale_to_x(scale, logical.left), x11_scale_to_x(scale, logical.right),
                        x11_scale_to_x(scale, logical.top), x11_scale_to_x(scale, logical.bottom)};
}

static int length_from_x(double scale, int x)
{
    return visible(x, x11_scale_from_x(scale, x));
}

static int placed_length(double scale, int start, int length)
{
    int end = x11_scale_from_x(scale, start + length);
    return visible(length, end - x11_scale_from_x(scale, start));
}

static int visible(int x, int length)
{
    return x > 0 && length < 1 ? 1 : length;
}

static int near_or(double scale, int logical, int hint, int fallback)
{
    return x11_scale_from_x(scale, hint) == logical ? hint : fallback;
}
