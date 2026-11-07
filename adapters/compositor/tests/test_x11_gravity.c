#include <xcb/xproto.h>

#include "check.h"
#include "xwayland/x11_gravity.h"

static const x11_framed_window framed = {{100, 200, 400, 300}, {4, 6, 24, 8}};

static void test_north_west_places_frame_corner(void)
{
    wm_point client = x11_gravity_place((wm_point){10, 20}, XCB_GRAVITY_NORTH_WEST,
                                        &framed.extents);
    CHECK_INT_EQ(14, client.x);
    CHECK_INT_EQ(44, client.y);
}

static void test_static_keeps_client_position(void)
{
    wm_point client = x11_gravity_place((wm_point){10, 20}, XCB_GRAVITY_STATIC, &framed.extents);
    CHECK_INT_EQ(10, client.x);
    CHECK_INT_EQ(20, client.y);
}

static void test_south_east_places_frame_far_corner(void)
{
    wm_point client = x11_gravity_place((wm_point){10, 20}, XCB_GRAVITY_SOUTH_EAST,
                                        &framed.extents);
    CHECK_INT_EQ(4, client.x);
    CHECK_INT_EQ(12, client.y);
}

static void test_center_splits_extents(void)
{
    wm_point client = x11_gravity_place((wm_point){10, 20}, XCB_GRAVITY_CENTER, &framed.extents);
    CHECK_INT_EQ(10 + (4 - 6 + 1) / 2, client.x);
    CHECK_INT_EQ(20 + (24 - 8 + 1) / 2, client.y);
}

static void test_move_request_uses_gravity(void)
{
    x11_configure request = {{0, 0, 0, 0}, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
                             XCB_GRAVITY_NORTH_WEST};
    CHECK_RECT_EQ(((wm_rect){4, 24, 400, 300}), x11_gravity_configure(&framed, &request));
}

static void test_resize_keeps_south_east_corner(void)
{
    x11_configure request = {{0, 0, 300, 250},
                             XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                             XCB_GRAVITY_SOUTH_EAST};
    CHECK_RECT_EQ(((wm_rect){200, 250, 300, 250}), x11_gravity_configure(&framed, &request));
}

static void test_resize_keeps_north_west_corner(void)
{
    x11_configure request = {{0, 0, 300, 250}, XCB_CONFIG_WINDOW_WIDTH, XCB_GRAVITY_NORTH_WEST};
    CHECK_RECT_EQ(((wm_rect){100, 200, 300, 300}), x11_gravity_configure(&framed, &request));
}

static void test_single_axis_move_keeps_other_axis(void)
{
    x11_configure request = {{50, 999, 0, 0}, XCB_CONFIG_WINDOW_X, XCB_GRAVITY_STATIC};
    CHECK_RECT_EQ(((wm_rect){50, 200, 400, 300}), x11_gravity_configure(&framed, &request));
}

int main(void)
{
    const check_case cases[] = {
        {"north_west_places_frame_corner", test_north_west_places_frame_corner},
        {"static_keeps_client_position", test_static_keeps_client_position},
        {"south_east_places_frame_far_corner", test_south_east_places_frame_far_corner},
        {"center_splits_extents", test_center_splits_extents},
        {"move_request_uses_gravity", test_move_request_uses_gravity},
        {"resize_keeps_south_east_corner", test_resize_keeps_south_east_corner},
        {"resize_keeps_north_west_corner", test_resize_keeps_north_west_corner},
        {"single_axis_move_keeps_other_axis", test_single_axis_move_keeps_other_axis},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
