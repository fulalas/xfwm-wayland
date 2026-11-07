#include "cursor_names.h"

static const cursor_name_list shape_names[] = {
    [WM_CURSOR_DEFAULT] = {{"default", "left_ptr", NULL, NULL}},
    [WM_CURSOR_MOVE] = {{"move", "fleur", "all-scroll", "default"}},
    [WM_CURSOR_RESIZE_TOP_LEFT] = {{"nw-resize", "top_left_corner", "default", NULL}},
    [WM_CURSOR_RESIZE_TOP] = {{"n-resize", "top_side", "default", NULL}},
    [WM_CURSOR_RESIZE_TOP_RIGHT] = {{"ne-resize", "top_right_corner", "default", NULL}},
    [WM_CURSOR_RESIZE_LEFT] = {{"w-resize", "left_side", "default", NULL}},
    [WM_CURSOR_RESIZE_RIGHT] = {{"e-resize", "right_side", "default", NULL}},
    [WM_CURSOR_RESIZE_BOTTOM_LEFT] = {{"sw-resize", "bottom_left_corner", "default", NULL}},
    [WM_CURSOR_RESIZE_BOTTOM] = {{"s-resize", "bottom_side", "default", NULL}},
    [WM_CURSOR_RESIZE_BOTTOM_RIGHT] = {{"se-resize", "bottom_right_corner", "default", NULL}},
    [WM_CURSOR_BUSY] = {{"progress", "left_ptr_watch", "watch", "default"}},
};

cursor_name_list cursor_names_for_shape(wm_cursor_shape shape)
{
    size_t count = sizeof(shape_names) / sizeof(shape_names[0]);
    if ((size_t) shape >= count) {
        return shape_names[WM_CURSOR_DEFAULT];
    }
    return shape_names[shape];
}
