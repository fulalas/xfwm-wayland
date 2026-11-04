#include "decor_private.h"

static void add_stretched(const decor_frame *frame, wm_decor_layout *layout, wm_rect area);
static wm_paint_op tiled_part_op(const decor_title *title, wm_rect area, int part);
static wm_pixmap_kind top_strip_kind(const decor_frame *frame, int part);
static wm_rect stretched_text_clip(const decor_plan *plan, wm_point position, wm_rect area);

void decor_paint_title(const decor_plan *plan, wm_decor_layout *layout)
{
    const decor_frame *frame = plan->frame;
    wm_rect area = decor_title_rect(frame);

    if (!plan->title->drawn) {
        return;
    }
    if (plan->title->stretched) {
        add_stretched(frame, layout, area);
        return;
    }
    for (int part = 0; part < WM_TITLE_PART_COUNT; part++) {
        wm_paint_op op = tiled_part_op(plan->title, area, part);
        op.pixmap = (wm_pixmap_ref) {WM_PIXMAP_TITLE, part, frame->state};
        decor_add_op(layout, frame->theme, op);
    }
}

void decor_paint_top_strip(const decor_plan *plan, wm_decor_layout *layout)
{
    const decor_frame *frame = plan->frame;
    wm_rect area = decor_top_strip_rect(frame, plan->title);

    if (!decor_top_strip_shown(frame, plan->title)) {
        return;
    }
    if (plan->title->stretched) {
        add_stretched(frame, layout, area);
        return;
    }
    for (int part = 0; part < WM_TITLE_PART_COUNT; part++) {
        wm_paint_op op = tiled_part_op(plan->title, area, part);
        op.pixmap = (wm_pixmap_ref) {top_strip_kind(frame, part), part, frame->state};
        decor_add_op(layout, frame->theme, op);
    }
}

wm_text_op decor_title_text(const decor_plan *plan)
{
    const decor_frame *frame = plan->frame;
    const decor_title *title = plan->title;
    wm_rect area = decor_title_rect(frame);
    wm_text_op text = {
        .visible = title->drawn && title->text_visible,
        .position = {area.x + title->text.x, area.y + title->text.y},
        .clip = area,
        .state = frame->state,
        .shadow = decor_title_shadow(frame),
    };

    if (title->stretched) {
        text.clip = stretched_text_clip(plan, text.position, area);
    }
    return text;
}

static void add_stretched(const decor_frame *frame, wm_decor_layout *layout, wm_rect area)
{
    wm_paint_op op = {
        .pixmap = {WM_PIXMAP_SIDE_STRETCH, WM_SIDE_TOP, frame->state},
        .dest = area,
        .origin = {area.x, area.y},
        .mode = WM_FILL_STRETCH_HORIZONTAL,
        .clip = area,
    };
    decor_add_op(layout, frame->theme, op);
}

static wm_paint_op tiled_part_op(const decor_title *title, wm_rect area, int part)
{
    int x = area.x + title->part_x[part];
    return (wm_paint_op) {
        .dest = {x, area.y, title->part_width[part], area.height},
        .origin = {x, area.y},
        .mode = WM_FILL_TILE,
        .clip = area,
    };
}

static wm_pixmap_kind top_strip_kind(const decor_frame *frame, int part)
{
    if (wm_size_is_empty(frame->theme->top[part][frame->state])) {
        return WM_PIXMAP_TITLE;
    }
    return WM_PIXMAP_TOP;
}

static wm_rect stretched_text_clip(const decor_plan *plan, wm_point position, wm_rect area)
{
    const decor_title *title = plan->title;
    wm_rect text_area = {
        position.x,
        position.y,
        title->right - title->part_width[DECOR_TITLE_RIGHT_EDGE],
        plan->frame->title_height,
    };
    return wm_rect_intersection(&area, &text_area);
}
