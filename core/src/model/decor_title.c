#include "decor_private.h"

enum {
    TOP_STRIP_TITLE_FRACTION = 10,
};

static void set_bounds(decor_title *title, const decor_frame *frame, const decor_buttons *buttons);
static int text_y(const decor_frame *frame);
static int vertical_offset(const decor_frame *frame);
static int top_height(const decor_frame *frame, int title_y);
static void set_full_width_parts(decor_title *title, const decor_frame *frame);
static int full_width_offset(const decor_frame *frame, int middle_width);
static void set_fitted_parts(decor_title *title, const decor_frame *frame);
static int fitted_start(const decor_title *title, const decor_frame *frame);
static void set_part_positions(decor_title *title);
static int part_width(const decor_frame *frame, int part);

decor_title decor_title_compute(const decor_frame *frame, const decor_buttons *buttons)
{
    decor_title title = {0};

    if (frame->top_width < 1) {
        return title;
    }
    title.drawn = true;
    title.stretched = !wm_size_is_empty(frame->theme->side_stretch[WM_SIDE_TOP][frame->state]);
    set_bounds(&title, frame, buttons);
    title.text.y = text_y(frame);
    title.top_height = top_height(frame, title.text.y);
    if (frame->config->title.full_width) {
        set_full_width_parts(&title, frame);
    } else {
        set_fitted_parts(&title, frame);
    }
    set_part_positions(&title);
    title.text_visible = title.stretched || title.part_width[DECOR_TITLE_MIDDLE] > 0;
    return title;
}

wm_title_shadow decor_title_shadow(const decor_frame *frame)
{
    const wm_title_config *config = &frame->config->title;
    if (frame->state == WM_FRAME_ACTIVE) {
        return config->shadow_active;
    }
    return config->shadow_inactive;
}

static void set_bounds(decor_title *title, const decor_frame *frame, const decor_buttons *buttons)
{
    int corner_left = frame->corner[WM_CORNER_TOP_LEFT].width;
    int corner_right = frame->size.width - frame->corner[WM_CORNER_TOP_RIGHT].width;
    int left = buttons->title_left;
    int right = buttons->title_right;

    if (left > right) {
        left = buttons->title_right;
        right = buttons->title_left;
    }
    left = wm_max(left, corner_left);
    right = wm_max(wm_min(right, corner_right), corner_left);
    title->left = left - corner_left;
    title->right = right - corner_left;
}

static int text_y(const decor_frame *frame)
{
    int text_height = frame->window->title_text_size.height;
    int y = vertical_offset(frame) + (frame->title_height - text_height) / 2;

    if (y + text_height > frame->title_height) {
        return wm_max(0, frame->title_height - text_height);
    }
    return y;
}

static int vertical_offset(const decor_frame *frame)
{
    const wm_title_config *config = &frame->config->title;
    if (frame->state == WM_FRAME_ACTIVE) {
        return config->vertical_offset_active;
    }
    return config->vertical_offset_inactive;
}

static int top_height(const decor_frame *frame, int title_y)
{
    wm_size top = frame->theme->top[DECOR_TITLE_RIGHT_EDGE][WM_FRAME_ACTIVE];
    int height;

    if (!wm_size_is_empty(top)) {
        return top.height;
    }
    if (frame->config->border_top > 0) {
        return frame->config->border_top;
    }
    height = frame->title_height / TOP_STRIP_TITLE_FRACTION + 1;
    if (height > title_y - 1) {
        return wm_max(title_y - 1, 0);
    }
    return height;
}

static void set_full_width_parts(decor_title *title, const decor_frame *frame)
{
    int *width = title->part_width;
    int offset;

    width[DECOR_TITLE_LEFT_FILL] = title->left;
    width[DECOR_TITLE_LEFT_EDGE] = part_width(frame, DECOR_TITLE_LEFT_EDGE);
    width[DECOR_TITLE_RIGHT_EDGE] = part_width(frame, DECOR_TITLE_RIGHT_EDGE);
    width[DECOR_TITLE_RIGHT_FILL] = frame->top_width - title->right;
    width[DECOR_TITLE_MIDDLE] = wm_max(0, frame->top_width - width[DECOR_TITLE_LEFT_FILL]
                                        - width[DECOR_TITLE_LEFT_EDGE]
                                        - width[DECOR_TITLE_RIGHT_EDGE]
                                        - width[DECOR_TITLE_RIGHT_FILL]);
    offset = wm_max(frame->config->title.horizontal_offset,
                    full_width_offset(frame, width[DECOR_TITLE_MIDDLE]));
    title->text.x = offset + width[DECOR_TITLE_LEFT_FILL] + width[DECOR_TITLE_LEFT_EDGE];
}

static int full_width_offset(const decor_frame *frame, int middle_width)
{
    const wm_title_config *config = &frame->config->title;
    int text_width = frame->window->title_text_size.width;

    switch (config->alignment) {
    case WM_ALIGN_LEFT:
        return config->horizontal_offset;
    case WM_ALIGN_RIGHT:
        return middle_width - text_width - config->horizontal_offset;
    default:
        return middle_width / 2 - text_width / 2;
    }
}

static void set_fitted_parts(decor_title *title, const decor_frame *frame)
{
    int *width = title->part_width;
    int shadow = (int) decor_title_shadow(frame);
    int available;

    width[DECOR_TITLE_LEFT_EDGE] = part_width(frame, DECOR_TITLE_LEFT_EDGE);
    width[DECOR_TITLE_RIGHT_EDGE] = part_width(frame, DECOR_TITLE_RIGHT_EDGE);
    width[DECOR_TITLE_RIGHT_FILL] = frame->top_width;
    available = frame->top_width - width[DECOR_TITLE_LEFT_EDGE] - width[DECOR_TITLE_RIGHT_EDGE];
    width[DECOR_TITLE_MIDDLE] = frame->window->title_text_size.width + shadow;
    if (width[DECOR_TITLE_MIDDLE] > available) {
        width[DECOR_TITLE_MIDDLE] = available;
    }
    width[DECOR_TITLE_MIDDLE] = wm_max(0, width[DECOR_TITLE_MIDDLE]);
    width[DECOR_TITLE_LEFT_FILL] = wm_max(title->left, fitted_start(title, frame));
    title->text.x = width[DECOR_TITLE_LEFT_FILL] + width[DECOR_TITLE_LEFT_EDGE];
}

static int fitted_start(const decor_title *title, const decor_frame *frame)
{
    const int *width = title->part_width;
    int offset = frame->config->title.horizontal_offset;
    int middle = width[DECOR_TITLE_MIDDLE];

    switch (frame->config->title.alignment) {
    case WM_ALIGN_LEFT:
        return title->left + offset;
    case WM_ALIGN_RIGHT:
        return title->right - width[DECOR_TITLE_LEFT_EDGE] - middle
               - width[DECOR_TITLE_RIGHT_EDGE] - offset;
    default:
        return title->left + (title->right - title->left) / 2 - middle / 2
               - width[DECOR_TITLE_LEFT_EDGE];
    }
}

static void set_part_positions(decor_title *title)
{
    const int *width = title->part_width;
    int *x = title->part_x;
    int right_edge_limit = title->right - width[DECOR_TITLE_RIGHT_EDGE];

    title->text_limit = right_edge_limit;

    x[DECOR_TITLE_LEFT_FILL] = 0;
    x[DECOR_TITLE_LEFT_EDGE] = wm_max(0, width[DECOR_TITLE_LEFT_FILL]);
    x[DECOR_TITLE_MIDDLE] = x[DECOR_TITLE_LEFT_EDGE] + width[DECOR_TITLE_LEFT_EDGE];
    x[DECOR_TITLE_RIGHT_EDGE] = x[DECOR_TITLE_MIDDLE] + wm_max(0, width[DECOR_TITLE_MIDDLE]);
    if (x[DECOR_TITLE_RIGHT_EDGE] > right_edge_limit) {
        x[DECOR_TITLE_RIGHT_EDGE] = right_edge_limit;
    }
    x[DECOR_TITLE_RIGHT_FILL] = x[DECOR_TITLE_RIGHT_EDGE] + width[DECOR_TITLE_RIGHT_EDGE];
}

static int part_width(const decor_frame *frame, int part)
{
    return frame->theme->title[part][frame->state].width;
}
