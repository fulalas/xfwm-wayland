#include <string.h>

#include "decor_private.h"

static void add_paint_ops(const decor_plan *plan, wm_decor_layout *layout);

void wm_decor_layout_compute(const wm_decor_input *input, wm_decor_layout *layout)
{
    decor_frame frame = decor_frame_compute(input);

    memset(layout, 0, sizeof *layout);
    layout->has_frame = frame.has_frame;
    layout->extents = frame.extents;
    layout->frame_size = frame.size;
    layout->title_text.state = frame.state;
    if (!frame.has_frame) {
        return;
    }
    decor_buttons buttons = decor_buttons_compute(&frame);
    decor_title title = decor_title_compute(&frame, &buttons);
    decor_plan plan = {&frame, &buttons, &title};
    add_paint_ops(&plan, layout);
    layout->title_text = decor_title_text(&plan);
    decor_parts_compute(&plan, layout);
}

static void add_paint_ops(const decor_plan *plan, wm_decor_layout *layout)
{
    bool borders = !plan->frame->borderless;

    if (borders) {
        decor_paint_sides(plan, layout);
    }
    decor_paint_title(plan, layout);
    if (borders) {
        decor_paint_top_strip(plan, layout);
        decor_paint_corners(plan, layout);
    }
    decor_paint_buttons(plan, layout);
}
