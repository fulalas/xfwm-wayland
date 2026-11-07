#include <stdbool.h>

#include "usable_area.h"

static bool range_hits(const strut_range *range, int start, int length);
static wm_rect cut_left(wm_rect usable, int boundary);
static wm_rect cut_right(wm_rect usable, int boundary);
static wm_rect cut_top(wm_rect usable, int boundary);
static wm_rect cut_bottom(wm_rect usable, int boundary);

wm_rect usable_area_apply_strut(wm_rect usable, const strut_context *context, const strut *s)
{
    const wm_rect *out = &context->output;
    const wm_rect *root = &context->root;
    if (range_hits(&s->left, out->y, out->height)) {
        usable = cut_left(usable, root->x + s->left.size);
    }
    if (range_hits(&s->right, out->y, out->height)) {
        usable = cut_right(usable, root->x + root->width - s->right.size);
    }
    if (range_hits(&s->top, out->x, out->width)) {
        usable = cut_top(usable, root->y + s->top.size);
    }
    if (range_hits(&s->bottom, out->x, out->width)) {
        usable = cut_bottom(usable, root->y + root->height - s->bottom.size);
    }
    return usable;
}

static bool range_hits(const strut_range *range, int start, int length)
{
    if (range->size <= 0) {
        return false;
    }
    return range->start < start + length && range->end >= start;
}

static wm_rect cut_left(wm_rect usable, int boundary)
{
    int right = usable.x + usable.width;
    if (boundary > usable.x) {
        usable.x = boundary < right ? boundary : right;
        usable.width = right - usable.x;
    }
    return usable;
}

static wm_rect cut_right(wm_rect usable, int boundary)
{
    if (boundary < usable.x + usable.width) {
        usable.width = boundary > usable.x ? boundary - usable.x : 0;
    }
    return usable;
}

static wm_rect cut_top(wm_rect usable, int boundary)
{
    int bottom = usable.y + usable.height;
    if (boundary > usable.y) {
        usable.y = boundary < bottom ? boundary : bottom;
        usable.height = bottom - usable.y;
    }
    return usable;
}

static wm_rect cut_bottom(wm_rect usable, int boundary)
{
    if (boundary < usable.y + usable.height) {
        usable.height = boundary > usable.y ? boundary - usable.y : 0;
    }
    return usable;
}
