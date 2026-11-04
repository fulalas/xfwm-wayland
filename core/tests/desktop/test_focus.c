#include "check.h"
#include "fake_input.h"

static const wm_rect small = {0, 0, 400, 300};

static void test_new_window_gets_focus(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    fake_map(desktop, 1, small);
    CHECK(world.focused == fake_handle(1));
    wm_desktop_destroy(desktop);
}

static void test_click_on_client_focuses_and_raises(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *first = fake_map(desktop, 1, small);
    fake_map(desktop, 2, small);
    fake_click click = {WM_TARGET_CLIENT, first, {800, 500}, WM_MOUSE_LEFT, 0};
    CHECK_INT_EQ(WM_FORWARD, fake_press(&world, desktop, &click));
    CHECK(world.focused == fake_handle(1));
    CHECK_INT_EQ(1, fake_stack_index(&world, fake_handle(1)));
    wm_desktop_destroy(desktop);
}

static void test_unmap_passes_focus_to_parent(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *parent = fake_map(desktop, 1, small);
    fake_map(desktop, 3, small);
    wm_window *dialog = fake_map_dialog(desktop, 2, parent);
    CHECK(world.focused == fake_handle(2));
    wm_window_unmap(desktop, dialog);
    CHECK(world.focused == fake_handle(1));
    wm_desktop_destroy(desktop);
}

static void test_unmap_passes_focus_to_top_most(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    fake_map(desktop, 1, small);
    fake_map(desktop, 2, small);
    wm_window *third = fake_map(desktop, 3, small);
    wm_window_unmap(desktop, third);
    CHECK(world.focused == fake_handle(2));
    wm_desktop_destroy(desktop);
}

static void test_focus_follows_mouse_after_delay(void)
{
    fake_world world;
    wm_config config = fake_config();
    config.focus.click_to_focus = false;
    config.focus.focus_delay_ms = 100;
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *first = fake_map(desktop, 1, small);
    fake_map(desktop, 2, small);
    fake_click where = {WM_TARGET_CLIENT, first, {10, 10}, 0, 0};
    fake_motion(&world, desktop, &where);
    fake_advance(&world, desktop, 99);
    CHECK(world.focused == fake_handle(2));
    fake_advance(&world, desktop, 1);
    CHECK(world.focused == fake_handle(1));
    wm_desktop_destroy(desktop);
}

static void test_prevented_focus_stealing_marks_urgent(void)
{
    fake_world world;
    wm_config config = fake_config();
    config.focus.prevent_focus_stealing = true;
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    fake_map(desktop, 1, small);
    wm_window_desc desc = fake_desc(2, small);
    desc.activation_allowed = false;
    wm_window *second = wm_window_map(desktop, &desc);
    CHECK(world.focused == fake_handle(1));
    CHECK(wm_window_current_view(second).urgent);
    CHECK_INT_EQ(0, fake_stack_index(&world, fake_handle(2)));
    wm_desktop_destroy(desktop);
}

static void test_modal_dialog_takes_focus_from_parent(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *parent = fake_map(desktop, 1, small);
    wm_window_desc modal = fake_dialog_desc(2, parent);
    modal.states = WM_STATE_MODAL;
    wm_window_map(desktop, &modal);
    fake_map(desktop, 3, (wm_rect) {0, 0, 100, 100});
    fake_click click = {WM_TARGET_CLIENT, parent, {800, 500}, WM_MOUSE_LEFT, 0};
    fake_press(&world, desktop, &click);
    CHECK(world.focused == fake_handle(2));
    wm_desktop_destroy(desktop);
}

static void test_window_without_focus_capability_is_skipped(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    fake_map(desktop, 1, small);
    wm_window_desc desc = fake_desc(2, small);
    desc.capabilities = WM_CAP_ALL & ~(uint32_t) WM_CAP_FOCUS;
    wm_window_map(desktop, &desc);
    CHECK(world.focused == fake_handle(1));
    wm_desktop_destroy(desktop);
}

static void test_pager_activation_switches_workspace(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    wm_desktop_switch_workspace(desktop, 2);
    wm_request_activate(desktop, window, WM_ACTIVATION_PAGER);
    CHECK_INT_EQ(0, world.workspaces.current);
    CHECK(world.focused == fake_handle(1));
    wm_desktop_destroy(desktop);
}

static void test_untrusted_activation_is_refused(void)
{
    fake_world world;
    wm_config config = fake_config();
    config.focus.prevent_focus_stealing = true;
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *first = fake_map(desktop, 1, small);
    fake_map(desktop, 2, small);
    wm_request_activate(desktop, first, WM_ACTIVATION_APPLICATION_UNTRUSTED);
    CHECK(world.focused == fake_handle(2));
    CHECK(wm_window_current_view(first).urgent);
    wm_desktop_destroy(desktop);
}

static void test_bring_activation_moves_window(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *window = fake_map(desktop, 1, small);
    wm_desktop_switch_workspace(desktop, 1);
    wm_request_activate(desktop, window, WM_ACTIVATION_APPLICATION);
    CHECK_INT_EQ(1, world.workspaces.current);
    CHECK_INT_EQ(1, wm_window_current_view(window).workspace);
    CHECK(wm_window_current_view(window).visible);
    wm_desktop_destroy(desktop);
}

int main(void)
{
    static const check_case cases[] = {
        {"new_window_gets_focus", test_new_window_gets_focus},
        {"click_on_client_focuses_and_raises", test_click_on_client_focuses_and_raises},
        {"unmap_passes_focus_to_parent", test_unmap_passes_focus_to_parent},
        {"unmap_passes_focus_to_top_most", test_unmap_passes_focus_to_top_most},
        {"focus_follows_mouse_after_delay", test_focus_follows_mouse_after_delay},
        {"prevented_focus_stealing_marks_urgent", test_prevented_focus_stealing_marks_urgent},
        {"modal_dialog_takes_focus_from_parent", test_modal_dialog_takes_focus_from_parent},
        {"window_without_focus_capability_is_skipped",
         test_window_without_focus_capability_is_skipped},
        {"pager_activation_switches_workspace", test_pager_activation_switches_workspace},
        {"untrusted_activation_is_refused", test_untrusted_activation_is_refused},
        {"bring_activation_moves_window", test_bring_activation_moves_window},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
