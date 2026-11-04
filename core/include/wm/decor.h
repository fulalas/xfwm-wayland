#ifndef WM_DECOR_H
#define WM_DECOR_H

#include <stdbool.h>
#include <stddef.h>

#include "wm/config.h"
#include "wm/geometry.h"

enum {
    WM_TITLE_PART_COUNT = 5,
    WM_DECOR_OP_MAX = 48,
};

typedef enum wm_frame_state {
    WM_FRAME_ACTIVE = 0,
    WM_FRAME_INACTIVE = 1,
    WM_FRAME_STATE_COUNT = 2,
} wm_frame_state;

typedef enum wm_corner {
    WM_CORNER_BOTTOM_LEFT = 0,
    WM_CORNER_BOTTOM_RIGHT,
    WM_CORNER_TOP_LEFT,
    WM_CORNER_TOP_RIGHT,
    WM_CORNER_COUNT
} wm_corner;

typedef enum wm_side {
    WM_SIDE_LEFT = 0,
    WM_SIDE_RIGHT,
    WM_SIDE_TOP,
    WM_SIDE_BOTTOM,
    WM_SIDE_COUNT
} wm_side;

typedef enum wm_button {
    WM_BUTTON_MENU = 0,
    WM_BUTTON_STICK,
    WM_BUTTON_SHADE,
    WM_BUTTON_HIDE,
    WM_BUTTON_MAXIMIZE,
    WM_BUTTON_CLOSE,
    WM_BUTTON_COUNT
} wm_button;

typedef enum wm_button_image {
    WM_IMAGE_ACTIVE = 0,
    WM_IMAGE_INACTIVE,
    WM_IMAGE_PRELIGHT,
    WM_IMAGE_INACTIVE_PRELIGHT,
    WM_IMAGE_PRESSED,
    WM_IMAGE_TOGGLED_ACTIVE,
    WM_IMAGE_TOGGLED_INACTIVE,
    WM_IMAGE_TOGGLED_PRELIGHT,
    WM_IMAGE_TOGGLED_INACTIVE_PRELIGHT,
    WM_IMAGE_TOGGLED_PRESSED,
    WM_IMAGE_COUNT
} wm_button_image;

typedef enum wm_button_status {
    WM_BUTTON_NORMAL = 0,
    WM_BUTTON_HOVERED,
    WM_BUTTON_PRESSED,
} wm_button_status;

typedef enum wm_part {
    WM_PART_NONE = -1,
    WM_PART_CORNER_BOTTOM_LEFT = WM_CORNER_BOTTOM_LEFT,
    WM_PART_CORNER_BOTTOM_RIGHT = WM_CORNER_BOTTOM_RIGHT,
    WM_PART_CORNER_TOP_LEFT = WM_CORNER_TOP_LEFT,
    WM_PART_CORNER_TOP_RIGHT = WM_CORNER_TOP_RIGHT,
    WM_PART_SIDE_LEFT = WM_CORNER_COUNT + WM_SIDE_LEFT,
    WM_PART_SIDE_RIGHT = WM_CORNER_COUNT + WM_SIDE_RIGHT,
    WM_PART_SIDE_TOP = WM_CORNER_COUNT + WM_SIDE_TOP,
    WM_PART_SIDE_BOTTOM = WM_CORNER_COUNT + WM_SIDE_BOTTOM,
    WM_PART_TITLE,
    WM_PART_BUTTON_FIRST,
    WM_PART_BUTTON_LAST = WM_PART_BUTTON_FIRST + WM_BUTTON_COUNT - 1,
} wm_part;

typedef struct wm_theme_metrics {
    wm_size side[WM_SIDE_COUNT][WM_FRAME_STATE_COUNT];
    wm_size side_stretch[WM_SIDE_COUNT][WM_FRAME_STATE_COUNT];
    wm_size corner[WM_CORNER_COUNT][WM_FRAME_STATE_COUNT];
    wm_size corner_stretch[WM_CORNER_COUNT][WM_FRAME_STATE_COUNT];
    wm_size button[WM_BUTTON_COUNT][WM_IMAGE_COUNT];
    wm_size title[WM_TITLE_PART_COUNT][WM_FRAME_STATE_COUNT];
    wm_size top[WM_TITLE_PART_COUNT][WM_FRAME_STATE_COUNT];
} wm_theme_metrics;

typedef struct wm_decor_window {
    wm_size client_size;
    wm_size title_text_size;
    wm_frame_state state;
    wm_button_status buttons[WM_BUTTON_COUNT];
    bool has_border;
    bool has_menu;
    bool has_stick;
    bool can_hide;
    bool can_maximize;
    bool has_close;
    bool has_icon;
    bool maximized_horizontally;
    bool maximized_vertically;
    bool fullscreen;
    bool shaded;
    bool sticky;
    bool hide_titlebar;
} wm_decor_window;

typedef struct wm_decor_input {
    const wm_theme_metrics *theme;
    const wm_frame_config *config;
    const wm_decor_window *window;
} wm_decor_input;

typedef enum wm_pixmap_kind {
    WM_PIXMAP_SIDE,
    WM_PIXMAP_SIDE_STRETCH,
    WM_PIXMAP_CORNER,
    WM_PIXMAP_CORNER_STRETCH,
    WM_PIXMAP_BUTTON,
    WM_PIXMAP_TITLE,
    WM_PIXMAP_TOP,
    WM_PIXMAP_APP_ICON,
} wm_pixmap_kind;

typedef struct wm_pixmap_ref {
    wm_pixmap_kind kind;
    int index;
    int image;
} wm_pixmap_ref;

typedef enum wm_fill_mode {
    WM_FILL_PLAIN,
    WM_FILL_TILE,
    WM_FILL_STRETCH_HORIZONTAL,
    WM_FILL_STRETCH_VERTICAL,
} wm_fill_mode;

typedef struct wm_paint_op {
    wm_pixmap_ref pixmap;
    wm_rect dest;
    wm_point origin;
    wm_fill_mode mode;
    wm_rect clip;
} wm_paint_op;

typedef struct wm_text_op {
    bool visible;
    wm_point position;
    wm_rect clip;
    wm_frame_state state;
    wm_title_shadow shadow;
} wm_text_op;

typedef struct wm_part_rect {
    wm_part part;
    wm_rect rect;
} wm_part_rect;

typedef struct wm_decor_layout {
    bool has_frame;
    wm_extents extents;
    wm_size frame_size;
    wm_paint_op ops[WM_DECOR_OP_MAX];
    size_t op_count;
    wm_text_op title_text;
    wm_part_rect parts[WM_DECOR_OP_MAX];
    size_t part_count;
} wm_decor_layout;

wm_extents wm_decor_extents(const wm_decor_input *input);
void wm_decor_layout_compute(const wm_decor_input *input, wm_decor_layout *layout);
wm_part wm_decor_part_at(const wm_decor_layout *layout, wm_point frame_point);
int wm_decor_title_height(const wm_theme_metrics *theme);

#endif
