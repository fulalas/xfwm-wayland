#include "check.h"
#include "session_apply.h"

static wm_window_view maximized_view(void)
{
    wm_window_view view = {0};
    view.client = (wm_rect){0, 0, 1920, 1080};
    view.restore_client = (wm_rect){100, 50, 640, 480};
    view.maximized = true;
    view.maximized_horizontally = true;
    view.maximized_vertically = true;
    view.minimized = true;
    view.above = true;
    view.workspace = 3;
    return view;
}

static void test_record_keeps_geometry_before_maximize(void)
{
    wm_window_view view = maximized_view();
    session_record record = session_record_from_view(&view);
    CHECK_RECT_EQ(((wm_rect) {100, 50, 640, 480}), record.geometry);
    CHECK_INT_EQ(WM_STATE_MAXIMIZED | WM_STATE_MINIMIZED | WM_STATE_ABOVE, record.states);
    CHECK(record.has_workspace);
    CHECK_INT_EQ(3, record.workspace);
}

static void test_sticky_window_has_no_workspace(void)
{
    wm_window_view view = maximized_view();
    view.sticky = true;
    session_record record = session_record_from_view(&view);
    CHECK(!record.has_workspace);
    CHECK((record.states & WM_STATE_STICKY) != 0);
}

static void test_same_state_ignores_name_and_time(void)
{
    wm_window_view view = maximized_view();
    session_record a = session_record_from_view(&view);
    session_record b = a;
    b.seen = 99;
    CHECK(session_record_same_state(&a, &b));
    b.geometry.x++;
    CHECK(!session_record_same_state(&a, &b));
}

static void test_launch_restores_geometry_and_states_but_not_minimized_or_workspace(void)
{
    wm_window_view view = maximized_view();
    session_record record = session_record_from_view(&view);
    wm_window_desc desc = {.states = WM_STATE_MODAL, .geometry = {0, 0, 10, 10}};
    session_apply_launch(&record, &desc);
    CHECK_RECT_EQ(((wm_rect) {100, 50, 640, 480}), desc.geometry);
    CHECK(desc.has_position);
    CHECK_INT_EQ(WM_STATE_MODAL | WM_STATE_MAXIMIZED | WM_STATE_ABOVE, desc.states);
    CHECK(!desc.has_workspace);
}

static void test_recovery_restores_everything(void)
{
    wm_window_view view = maximized_view();
    session_record record = session_record_from_view(&view);
    wm_window_desc desc = {0};
    session_apply_recovery(&record, &desc);
    CHECK_INT_EQ(WM_STATE_MAXIMIZED | WM_STATE_MINIMIZED | WM_STATE_ABOVE, desc.states);
    CHECK(desc.has_workspace);
    CHECK_INT_EQ(3, desc.workspace);
}

static void test_record_keeps_maximize_in_one_direction(void)
{
    wm_window_view view = {0};
    view.maximized_vertically = true;
    session_record record = session_record_from_view(&view);
    CHECK_INT_EQ(WM_STATE_MAXIMIZED_VERT, record.states);
}

int main(void)
{
    static const check_case cases[] = {
        {"record_keeps_geometry_before_maximize", test_record_keeps_geometry_before_maximize},
        {"sticky_window_has_no_workspace", test_sticky_window_has_no_workspace},
        {"same_state_ignores_name_and_time", test_same_state_ignores_name_and_time},
        {"launch_restores_geometry_and_states_but_not_minimized_or_workspace",
         test_launch_restores_geometry_and_states_but_not_minimized_or_workspace},
        {"recovery_restores_everything", test_recovery_restores_everything},
        {"record_keeps_maximize_in_one_direction", test_record_keeps_maximize_in_one_direction},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
