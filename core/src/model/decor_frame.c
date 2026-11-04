#include "decor_private.h"

const wm_corner decor_corner_order[WM_CORNER_COUNT] = {
    WM_CORNER_TOP_LEFT,
    WM_CORNER_TOP_RIGHT,
    WM_CORNER_BOTTOM_LEFT,
    WM_CORNER_BOTTOM_RIGHT,
};

static wm_size frame_size(const wm_decor_input *input, const wm_extents *extents);
static wm_size corner_size(const wm_decor_input *input, wm_corner corner);
static bool is_top_corner(wm_corner corner);
static int corner_height_limit(const decor_frame *frame, wm_corner corner);
static wm_rect left_side_rect(const decor_frame *frame);
static wm_rect right_side_rect(const decor_frame *frame);
static wm_rect bottom_side_rect(const decor_frame *frame);

decor_frame decor_frame_compute(const wm_decor_input *input)
{
    decor_frame frame = {
        .theme = input->theme,
        .config = input->config,
        .window = input->window,
        .state = input->window->state,
        .has_frame = decor_has_frame(input),
        .borderless = decor_is_borderless(input),
        .extents = wm_decor_extents(input),
        .border_top = decor_border_top(input),
        .title_height = wm_decor_title_height(input->theme),
    };
    frame.size = frame_size(input, &frame.extents);
    for (int corner = 0; corner < WM_CORNER_COUNT; corner++) {
        frame.corner[corner] = corner_size(input, (wm_corner) corner);
    }
    frame.top_width = frame.size.width - frame.corner[WM_CORNER_TOP_LEFT].width
                      - frame.corner[WM_CORNER_TOP_RIGHT].width;
    return frame;
}

wm_rect decor_corner_rect(const decor_frame *frame, wm_corner corner)
{
    wm_size size = frame->corner[corner];
    bool right = corner == WM_CORNER_TOP_RIGHT || corner == WM_CORNER_BOTTOM_RIGHT;
    return (wm_rect) {
        right ? frame->size.width - size.width : 0,
        is_top_corner(corner) ? 0 : frame->size.height - size.height,
        size.width,
        size.height,
    };
}

wm_rect decor_corner_visible_rect(const decor_frame *frame, wm_corner corner)
{
    wm_rect rect = decor_corner_rect(frame, corner);
    int limit = corner_height_limit(frame, corner);
    int excess = rect.height - limit;

    if (excess <= 0) {
        return rect;
    }
    rect.height = limit;
    if (!is_top_corner(corner)) {
        rect.y += excess;
    }
    return rect;
}

wm_rect decor_side_rect(const decor_frame *frame, wm_side side)
{
    switch (side) {
    case WM_SIDE_LEFT:
        return left_side_rect(frame);
    case WM_SIDE_RIGHT:
        return right_side_rect(frame);
    case WM_SIDE_BOTTOM:
        return bottom_side_rect(frame);
    default:
        return (wm_rect) {0, 0, 0, 0};
    }
}

wm_rect decor_title_rect(const decor_frame *frame)
{
    return (wm_rect) {
        frame->corner[WM_CORNER_TOP_LEFT].width,
        -frame->border_top,
        frame->top_width,
        frame->title_height,
    };
}

wm_rect decor_top_strip_rect(const decor_frame *frame, const decor_title *title)
{
    return (wm_rect) {
        frame->corner[WM_CORNER_TOP_LEFT].width,
        0,
        frame->top_width,
        title->top_height,
    };
}

bool decor_top_strip_shown(const decor_frame *frame, const decor_title *title)
{
    return !frame->borderless && title->drawn && title->top_height > 0;
}

static wm_size frame_size(const wm_decor_input *input, const wm_extents *extents)
{
    const wm_decor_window *window = input->window;
    int vertical_border = extents->top + extents->bottom;
    int width = window->client_size.width + extents->left + extents->right;

    if (window->shaded && window->has_border && !window->fullscreen) {
        return (wm_size) {width, vertical_border};
    }
    return (wm_size) {width, window->client_size.height + vertical_border};
}

static wm_size corner_size(const wm_decor_input *input, wm_corner corner)
{
    wm_frame_state state = input->window->state;
    if (decor_is_borderless(input)) {
        return (wm_size) {0, 0};
    }
    return decor_pick_size(input->theme->corner_stretch[corner][state],
                           input->theme->corner[corner][state]);
}

static bool is_top_corner(wm_corner corner)
{
    return corner == WM_CORNER_TOP_LEFT || corner == WM_CORNER_TOP_RIGHT;
}

static int corner_height_limit(const decor_frame *frame, wm_corner corner)
{
    int facing_border = is_top_corner(corner) ? frame->extents.bottom : frame->extents.top;
    return frame->size.height - facing_border + 1;
}

static wm_rect left_side_rect(const decor_frame *frame)
{
    int height = frame->size.height - frame->extents.top
                 - frame->corner[WM_CORNER_BOTTOM_LEFT].height;
    return (wm_rect) {0, frame->extents.top, frame->extents.left, height};
}

static wm_rect right_side_rect(const decor_frame *frame)
{
    int height = frame->size.height - frame->extents.top
                 - frame->corner[WM_CORNER_BOTTOM_RIGHT].height;
    int x = frame->size.width - frame->extents.right;
    return (wm_rect) {x, frame->extents.top, frame->extents.right, height};
}

static wm_rect bottom_side_rect(const decor_frame *frame)
{
    int left = frame->corner[WM_CORNER_BOTTOM_LEFT].width;
    int width = frame->size.width - left - frame->corner[WM_CORNER_BOTTOM_RIGHT].width;
    int y = frame->size.height - frame->extents.bottom;
    return (wm_rect) {left, y, width, frame->extents.bottom};
}
