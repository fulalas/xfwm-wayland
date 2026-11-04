#include "decor_private.h"

static void add_part(wm_decor_layout *layout, wm_part part, wm_rect rect);
static void add_button_parts(const decor_plan *plan, wm_decor_layout *layout);
static void add_corner_parts(const decor_plan *plan, wm_decor_layout *layout);
static void add_side_parts(const decor_plan *plan, wm_decor_layout *layout);

void decor_parts_compute(const decor_plan *plan, wm_decor_layout *layout)
{
    const decor_frame *frame = plan->frame;

    add_button_parts(plan, layout);
    if (!frame->borderless) {
        add_corner_parts(plan, layout);
    }
    if (decor_top_strip_shown(frame, plan->title)) {
        add_part(layout, WM_PART_SIDE_TOP, decor_top_strip_rect(frame, plan->title));
    }
    add_part(layout, WM_PART_TITLE, decor_title_rect(frame));
    if (!frame->borderless) {
        add_side_parts(plan, layout);
    }
}

wm_part wm_decor_part_at(const wm_decor_layout *layout, wm_point frame_point)
{
    for (size_t i = 0; i < layout->part_count; i++) {
        if (wm_rect_contains(&layout->parts[i].rect, frame_point)) {
            return layout->parts[i].part;
        }
    }
    return WM_PART_NONE;
}

static void add_part(wm_decor_layout *layout, wm_part part, wm_rect rect)
{
    if (layout->part_count >= WM_DECOR_OP_MAX || wm_rect_is_empty(&rect)) {
        return;
    }
    layout->parts[layout->part_count++] = (wm_part_rect) {part, rect};
}

static void add_button_parts(const decor_plan *plan, wm_decor_layout *layout)
{
    for (int button = 0; button < WM_BUTTON_COUNT; button++) {
        if (plan->buttons->visible[button]) {
            add_part(layout, (wm_part) (WM_PART_BUTTON_FIRST + button),
                     plan->buttons->rect[button]);
        }
    }
}

static void add_corner_parts(const decor_plan *plan, wm_decor_layout *layout)
{
    for (int i = 0; i < WM_CORNER_COUNT; i++) {
        wm_corner corner = decor_corner_order[i];
        add_part(layout, (wm_part) (WM_PART_CORNER_BOTTOM_LEFT + corner),
                 decor_corner_visible_rect(plan->frame, corner));
    }
}

static void add_side_parts(const decor_plan *plan, wm_decor_layout *layout)
{
    const decor_frame *frame = plan->frame;

    if (!frame->window->shaded) {
        add_part(layout, WM_PART_SIDE_LEFT, decor_side_rect(frame, WM_SIDE_LEFT));
        add_part(layout, WM_PART_SIDE_RIGHT, decor_side_rect(frame, WM_SIDE_RIGHT));
    }
    add_part(layout, WM_PART_SIDE_BOTTOM, decor_side_rect(frame, WM_SIDE_BOTTOM));
}
