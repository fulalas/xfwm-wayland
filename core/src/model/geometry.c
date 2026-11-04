#include "wm/geometry.h"

static int segment_overlap_length(int start_a, int end_a, int start_b, int end_b);

bool wm_rect_is_empty(const wm_rect *rect)
{
    return rect->width <= 0 || rect->height <= 0;
}

bool wm_rect_contains(const wm_rect *rect, wm_point point)
{
    if (wm_rect_is_empty(rect)) {
        return false;
    }
    return point.x >= rect->x && point.x < wm_rect_right(rect) && point.y >= rect->y
           && point.y < wm_rect_bottom(rect);
}

bool wm_rect_intersects(const wm_rect *a, const wm_rect *b)
{
    return wm_rect_overlap_area(a, b) > 0;
}

bool wm_rect_equals(const wm_rect *a, const wm_rect *b)
{
    return a->x == b->x && a->y == b->y && a->width == b->width && a->height == b->height;
}

wm_rect wm_rect_intersection(const wm_rect *a, const wm_rect *b)
{
    int x = wm_max(a->x, b->x);
    int y = wm_max(a->y, b->y);
    int right = wm_min(wm_rect_right(a), wm_rect_right(b));
    int bottom = wm_min(wm_rect_bottom(a), wm_rect_bottom(b));
    return (wm_rect) {x, y, wm_max(right - x, 0), wm_max(bottom - y, 0)};
}

wm_rect wm_rect_union(const wm_rect *a, const wm_rect *b)
{
    int x = wm_min(a->x, b->x);
    int y = wm_min(a->y, b->y);
    int right = wm_max(wm_rect_right(a), wm_rect_right(b));
    int bottom = wm_max(wm_rect_bottom(a), wm_rect_bottom(b));
    return (wm_rect) {x, y, right - x, bottom - y};
}

int wm_rect_right(const wm_rect *rect)
{
    return rect->x + rect->width;
}

int wm_rect_bottom(const wm_rect *rect)
{
    return rect->y + rect->height;
}

long wm_rect_overlap_area(const wm_rect *a, const wm_rect *b)
{
    if (wm_rect_is_empty(a) || wm_rect_is_empty(b)) {
        return 0;
    }
    int width = segment_overlap_length(a->x, wm_rect_right(a), b->x, wm_rect_right(b));
    int height = segment_overlap_length(a->y, wm_rect_bottom(a), b->y, wm_rect_bottom(b));
    return (long) width * (long) height;
}

wm_point wm_rect_center(const wm_rect *rect)
{
    return (wm_point) {rect->x + rect->width / 2, rect->y + rect->height / 2};
}

wm_rect wm_rect_shrink(const wm_rect *rect, const wm_extents *extents)
{
    return (wm_rect) {
        rect->x + extents->left,
        rect->y + extents->top,
        rect->width - extents->left - extents->right,
        rect->height - extents->top - extents->bottom,
    };
}

wm_rect wm_rect_grow(const wm_rect *rect, const wm_extents *extents)
{
    return (wm_rect) {
        rect->x - extents->left,
        rect->y - extents->top,
        rect->width + extents->left + extents->right,
        rect->height + extents->top + extents->bottom,
    };
}

bool wm_segments_overlap(int start_a, int end_a, int start_b, int end_b)
{
    return segment_overlap_length(start_a, end_a, start_b, end_b) > 0;
}

int wm_clamp(int value, int min, int max)
{
    if (value > max) {
        return max;
    }
    if (value < min) {
        return min;
    }
    return value;
}

int wm_min(int a, int b)
{
    return a < b ? a : b;
}

int wm_max(int a, int b)
{
    return a > b ? a : b;
}

bool wm_size_equals(wm_size a, wm_size b)
{
    return a.width == b.width && a.height == b.height;
}

bool wm_size_is_empty(wm_size size)
{
    return size.width <= 0 || size.height <= 0;
}

static int segment_overlap_length(int start_a, int end_a, int start_b, int end_b)
{
    int start = wm_max(start_a, start_b);
    int end = wm_min(end_a, end_b);
    return end > start ? end - start : 0;
}
