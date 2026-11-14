#include <string.h>
#include <wayland-server-protocol.h>

#include "check.h"
#include "cursor/cursor_names.h"

static bool ends_with_default(cursor_name_list list)
{
    const char *last = NULL;
    for (int i = 0; i < CURSOR_NAME_CHOICES && list.names[i] != NULL; i++) {
        last = list.names[i];
    }
    return last != NULL && (strcmp(last, "default") == 0 || strcmp(last, "left_ptr") == 0);
}

static void test_resize_shapes_use_css_names_first(void)
{
    CHECK(strcmp("nw-resize", cursor_names_for_shape(WM_CURSOR_RESIZE_TOP_LEFT).names[0]) == 0);
    CHECK(strcmp("s-resize", cursor_names_for_shape(WM_CURSOR_RESIZE_BOTTOM).names[0]) == 0);
    CHECK(strcmp("e-resize", cursor_names_for_shape(WM_CURSOR_RESIZE_RIGHT).names[0]) == 0);
}

static void test_move_falls_back_to_fleur(void)
{
    cursor_name_list list = cursor_names_for_shape(WM_CURSOR_MOVE);
    CHECK(strcmp("move", list.names[0]) == 0);
    CHECK(strcmp("fleur", list.names[1]) == 0);
}

static void test_every_shape_ends_with_default(void)
{
    for (int shape = WM_CURSOR_DEFAULT; shape <= WM_CURSOR_RESIZE_BOTTOM_RIGHT; shape++) {
        CHECK(ends_with_default(cursor_names_for_shape((wm_cursor_shape) shape)));
    }
}

static void test_drag_actions_end_with_default(void)
{
    for (uint32_t action = WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE;
         action <= WL_DATA_DEVICE_MANAGER_DND_ACTION_ASK; action++) {
        CHECK(ends_with_default(cursor_names_for_drag_action(action)));
    }
    cursor_name_list copy = cursor_names_for_drag_action(WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY);
    cursor_name_list none = cursor_names_for_drag_action(WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE);
    CHECK(strcmp("copy", copy.names[0]) == 0);
    CHECK(strcmp("no-drop", none.names[0]) == 0);
}

static void test_unknown_shape_uses_default(void)
{
    cursor_name_list list = cursor_names_for_shape((wm_cursor_shape) 99);
    CHECK(strcmp("default", list.names[0]) == 0);
}

int main(void)
{
    const check_case cases[] = {
        {"resize_shapes_use_css_names_first", test_resize_shapes_use_css_names_first},
        {"move_falls_back_to_fleur", test_move_falls_back_to_fleur},
        {"every_shape_ends_with_default", test_every_shape_ends_with_default},
        {"unknown_shape_uses_default", test_unknown_shape_uses_default},
        {"drag_actions_end_with_default", test_drag_actions_end_with_default},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
