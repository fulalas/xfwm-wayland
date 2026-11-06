#include "decor_private.h"

static wm_size source_size(const wm_theme_metrics *theme, wm_pixmap_ref pixmap);
static void add_side(const decor_frame *frame, wm_decor_layout *layout, wm_side side);
static void add_corner(const decor_frame *frame, wm_decor_layout *layout, wm_corner corner);

bool decor_add_op(wm_decor_layout *layout, const wm_theme_metrics *theme, wm_paint_op op)
{
    if (layout->op_count >= WM_DECOR_OP_MAX) {
        return false;
    }
    if (wm_rect_is_empty(&op.dest) || wm_rect_is_empty(&op.clip)) {
        return false;
    }
    if (wm_size_is_empty(source_size(theme, op.pixmap))) {
        return false;
    }
    layout->ops[layout->op_count++] = op;
    return true;
}

void decor_paint_sides(const decor_plan *plan, wm_decor_layout *layout)
{
    if (!plan->frame->window->shaded) {
        add_side(plan->frame, layout, WM_SIDE_LEFT);
        add_side(plan->frame, layout, WM_SIDE_RIGHT);
    }
    add_side(plan->frame, layout, WM_SIDE_BOTTOM);
}

void decor_paint_corners(const decor_plan *plan, wm_decor_layout *layout)
{
    for (int i = 0; i < WM_CORNER_COUNT; i++) {
        add_corner(plan->frame, layout, decor_corner_order[i]);
    }
}

static wm_size source_size(const wm_theme_metrics *theme, wm_pixmap_ref pixmap)
{
    switch (pixmap.kind) {
    case WM_PIXMAP_SIDE:
        return theme->side[pixmap.index][pixmap.image];
    case WM_PIXMAP_SIDE_STRETCH:
        return theme->side_stretch[pixmap.index][pixmap.image];
    case WM_PIXMAP_CORNER:
        return theme->corner[pixmap.index][pixmap.image];
    case WM_PIXMAP_CORNER_STRETCH:
        return theme->corner_stretch[pixmap.index][pixmap.image];
    case WM_PIXMAP_BUTTON:
        return theme->button[pixmap.index][pixmap.image];
    case WM_PIXMAP_TITLE:
        return theme->title[pixmap.index][pixmap.image];
    case WM_PIXMAP_TOP:
        return theme->top[pixmap.index][pixmap.image];
    default:
        return (wm_size) {1, 1};
    }
}

static void add_side(const decor_frame *frame, wm_decor_layout *layout, wm_side side)
{
    wm_rect rect = decor_side_rect(frame, side);
    wm_paint_op op = {
        .pixmap = {WM_PIXMAP_SIDE, side, frame->state},
        .dest = rect,
        .origin = {rect.x, rect.y},
        .mode = WM_FILL_TILE,
        .clip = rect,
    };

    if (!wm_size_is_empty(frame->theme->side_stretch[side][frame->state])) {
        op.pixmap.kind = WM_PIXMAP_SIDE_STRETCH;
        op.mode = side == WM_SIDE_BOTTOM ? WM_FILL_STRETCH_HORIZONTAL : WM_FILL_STRETCH_VERTICAL;
    }
    decor_add_op(layout, frame->theme, op);
}

static void add_corner(const decor_frame *frame, wm_decor_layout *layout, wm_corner corner)
{
    wm_rect rect = decor_corner_rect(frame, corner);
    wm_paint_op op = {
        .pixmap = {WM_PIXMAP_CORNER, corner, frame->state},
        .dest = rect,
        .origin = {rect.x, rect.y},
        .mode = WM_FILL_PLAIN,
        .clip = decor_corner_visible_rect(frame, corner),
    };

    if (!wm_size_is_empty(frame->theme->corner_stretch[corner][frame->state])) {
        op.pixmap.kind = WM_PIXMAP_CORNER_STRETCH;
    }
    decor_add_op(layout, frame->theme, op);
}
