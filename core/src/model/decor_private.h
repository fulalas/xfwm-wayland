#ifndef DECOR_PRIVATE_H
#define DECOR_PRIVATE_H

#include <stdbool.h>

#include "wm/decor.h"

enum {
    DECOR_TITLE_LEFT_FILL = 0,
    DECOR_TITLE_LEFT_EDGE,
    DECOR_TITLE_MIDDLE,
    DECOR_TITLE_RIGHT_EDGE,
    DECOR_TITLE_RIGHT_FILL,
};

typedef struct decor_frame {
    const wm_theme_metrics *theme;
    const wm_frame_config *config;
    const wm_decor_window *window;
    wm_frame_state state;
    bool has_frame;
    bool borderless;
    wm_extents extents;
    wm_size size;
    int border_top;
    int title_height;
    int top_width;
    wm_size corner[WM_CORNER_COUNT];
} decor_frame;

typedef struct decor_buttons {
    bool visible[WM_BUTTON_COUNT];
    wm_rect rect[WM_BUTTON_COUNT];
    int title_left;
    int title_right;
} decor_buttons;

typedef struct decor_title {
    bool drawn;
    bool stretched;
    bool text_visible;
    int top_height;
    int left;
    int right;
    int text_limit;
    wm_point text;
    int part_x[WM_TITLE_PART_COUNT];
    int part_width[WM_TITLE_PART_COUNT];
} decor_title;

typedef struct decor_plan {
    const decor_frame *frame;
    const decor_buttons *buttons;
    const decor_title *title;
} decor_plan;

extern const wm_corner decor_corner_order[WM_CORNER_COUNT];

wm_size decor_pick_size(wm_size preferred, wm_size fallback);
bool decor_is_borderless(const wm_decor_input *input);
int decor_border_top(const wm_decor_input *input);
int decor_button_offset(const decor_frame *frame);

decor_frame decor_frame_compute(const wm_decor_input *input);
wm_rect decor_corner_rect(const decor_frame *frame, wm_corner corner);
wm_rect decor_corner_visible_rect(const decor_frame *frame, wm_corner corner);
wm_rect decor_side_rect(const decor_frame *frame, wm_side side);
wm_rect decor_title_rect(const decor_frame *frame);
wm_rect decor_top_strip_rect(const decor_frame *frame, const decor_title *title);
bool decor_top_strip_shown(const decor_frame *frame, const decor_title *title);

decor_buttons decor_buttons_compute(const decor_frame *frame);
int decor_menu_button_x(const decor_frame *frame);
wm_button_image decor_button_image(const decor_frame *frame, wm_button button);

decor_title decor_title_compute(const decor_frame *frame, const decor_buttons *buttons);
wm_title_shadow decor_title_shadow(const decor_frame *frame);
wm_text_op decor_title_text(const decor_plan *plan);

bool decor_add_op(wm_decor_layout *layout, const wm_theme_metrics *theme, wm_paint_op op);
void decor_paint_sides(const decor_plan *plan, wm_decor_layout *layout);
void decor_paint_corners(const decor_plan *plan, wm_decor_layout *layout);
void decor_paint_title(const decor_plan *plan, wm_decor_layout *layout);
void decor_paint_top_strip(const decor_plan *plan, wm_decor_layout *layout);
void decor_paint_buttons(const decor_plan *plan, wm_decor_layout *layout);
void decor_parts_compute(const decor_plan *plan, wm_decor_layout *layout);

#endif
