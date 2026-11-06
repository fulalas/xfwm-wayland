#include <string.h>

#include "check.h"
#include "ewmh_payload.h"

static void test_sticky_window_is_on_all_desktops(void)
{
    wm_window_view view = {.workspace = 2, .sticky = true};
    CHECK(ewmh_window_desktop(&view) == EWMH_ALL_DESKTOPS);
}

static void test_window_desktop_is_its_workspace(void)
{
    wm_window_view view = {.workspace = 2};
    CHECK_INT_EQ(2, ewmh_window_desktop(&view));
}

static void test_frame_extents_order_and_clamp(void)
{
    wm_extents extents = {4, 5, 24, -1};
    ewmh_frame_extents payload = ewmh_frame_extents_from(&extents);
    CHECK_INT_EQ(4, payload.values[0]);
    CHECK_INT_EQ(5, payload.values[1]);
    CHECK_INT_EQ(24, payload.values[2]);
    CHECK_INT_EQ(0, payload.values[3]);
}

static void test_maximize_gives_both_directions(void)
{
    ewmh_action_list list = ewmh_actions(WM_ALLOW_MAXIMIZE | WM_ALLOW_CLOSE);
    CHECK_INT_EQ(3, list.count);
    CHECK_INT_EQ(ATOM_NET_WM_ACTION_MAXIMIZE_HORZ, list.atoms[0]);
    CHECK_INT_EQ(ATOM_NET_WM_ACTION_MAXIMIZE_VERT, list.atoms[1]);
    CHECK_INT_EQ(ATOM_NET_WM_ACTION_CLOSE, list.atoms[2]);
}

static void test_names_are_nul_separated(void)
{
    wm_workspaces_view view = {.count = 3, .names = {"One", NULL, "Three"}};
    ewmh_desktop_names names = ewmh_names(&view);
    CHECK_INT_EQ(11, names.length);
    CHECK(memcmp(names.bytes, "One\0\0Three\0", 11) == 0);
}

static void test_workarea_of_single_output_is_usable_area(void)
{
    wm_output_info outputs[] = {{{0, 0, 1920, 1080}, {0, 30, 1920, 1050}, false}};
    ewmh_screen screen = {{0, 0, 1920, 1080}, outputs, 1};
    CHECK_RECT_EQ(((wm_rect){0, 30, 1920, 1050}), ewmh_workarea(&screen));
}

static void test_workarea_keeps_panels_of_every_screen_edge(void)
{
    wm_output_info outputs[] = {
        {{0, 0, 1920, 1080}, {0, 30, 1920, 1050}, false},
        {{1920, 0, 1280, 1024}, {1920, 0, 1230, 1024}, false},
    };
    ewmh_screen screen = {{0, 0, 3200, 1080}, outputs, 2};
    CHECK_RECT_EQ(((wm_rect){0, 30, 3150, 1050}), ewmh_workarea(&screen));
}

static void test_inner_panel_does_not_shrink_workarea(void)
{
    wm_output_info outputs[] = {
        {{0, 0, 1920, 1080}, {0, 0, 1880, 1080}, false},
        {{1920, 0, 1920, 1080}, {1920, 0, 1920, 1080}, false},
    };
    ewmh_screen screen = {{0, 0, 3840, 1080}, outputs, 2};
    CHECK_RECT_EQ(((wm_rect){0, 0, 3840, 1080}), ewmh_workarea(&screen));
}

static void test_names_split_on_nul(void)
{
    const char bytes[] = "Web\0Mail\0Code";
    const char *names[4];
    size_t count = ewmh_split_names(bytes, sizeof(bytes) - 1, names, 4);
    CHECK_INT_EQ(3, (long) count);
    CHECK(strcmp(names[1], "Mail") == 0);
    CHECK(strcmp(names[2], "Code") == 0);
}

int main(void)
{
    const check_case cases[] = {
        {"names_split_on_nul", test_names_split_on_nul},
        {"sticky_window_is_on_all_desktops", test_sticky_window_is_on_all_desktops},
        {"window_desktop_is_its_workspace", test_window_desktop_is_its_workspace},
        {"frame_extents_order_and_clamp", test_frame_extents_order_and_clamp},
        {"maximize_gives_both_directions", test_maximize_gives_both_directions},
        {"names_are_nul_separated", test_names_are_nul_separated},
        {"workarea_of_single_output_is_usable_area", test_workarea_of_single_output_is_usable_area},
        {"workarea_keeps_panels_of_every_screen_edge",
         test_workarea_keeps_panels_of_every_screen_edge},
        {"inner_panel_does_not_shrink_workarea", test_inner_panel_does_not_shrink_workarea},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
