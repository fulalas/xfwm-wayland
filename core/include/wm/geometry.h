#ifndef WM_GEOMETRY_H
#define WM_GEOMETRY_H

#include <stdbool.h>

typedef struct wm_point {
    int x;
    int y;
} wm_point;

typedef struct wm_size {
    int width;
    int height;
} wm_size;

typedef struct wm_rect {
    int x;
    int y;
    int width;
    int height;
} wm_rect;

typedef struct wm_extents {
    int left;
    int right;
    int top;
    int bottom;
} wm_extents;

enum wm_edge {
    WM_EDGE_NONE = 0,
    WM_EDGE_TOP = 1 << 0,
    WM_EDGE_BOTTOM = 1 << 1,
    WM_EDGE_LEFT = 1 << 2,
    WM_EDGE_RIGHT = 1 << 3,
};

bool wm_rect_is_empty(const wm_rect *rect);
bool wm_rect_contains(const wm_rect *rect, wm_point point);
bool wm_rect_intersects(const wm_rect *a, const wm_rect *b);
bool wm_rect_equals(const wm_rect *a, const wm_rect *b);
wm_rect wm_rect_intersection(const wm_rect *a, const wm_rect *b);
wm_rect wm_rect_union(const wm_rect *a, const wm_rect *b);
int wm_rect_right(const wm_rect *rect);
int wm_rect_bottom(const wm_rect *rect);
long wm_rect_overlap_area(const wm_rect *a, const wm_rect *b);
wm_point wm_rect_center(const wm_rect *rect);
wm_rect wm_rect_shrink(const wm_rect *rect, const wm_extents *extents);
wm_rect wm_rect_grow(const wm_rect *rect, const wm_extents *extents);
bool wm_segments_overlap(int start_a, int end_a, int start_b, int end_b);
int wm_clamp(int value, int min, int max);
int wm_min(int a, int b);
int wm_max(int a, int b);
bool wm_size_equals(wm_size a, wm_size b);
bool wm_size_is_empty(wm_size size);

#endif
