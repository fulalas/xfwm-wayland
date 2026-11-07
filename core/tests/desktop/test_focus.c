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
    fake_click click = {WM_TARGET_CLIENT, first, {800, 500}, WM_MOUSE_LEFT, 0, -1};
    CHECK_INT_EQ(WM_FORWARD, fake_press(&world, desktop, &click));
    CHECK(world.focused == fake_handle(1));
    CHECK_INT_EQ(1, fake_stack_index(&world, fake_handle(1)));
    wm_desktop_destroy(desktop);
}

static void test_click_on_desktop_clears_focus(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    fake_map(desktop, 1, small);
    fake_click click = {WM_TARGET_DESKTOP, NULL, {800, 500}, WM_MOUSE_LEFT, 0, -1};
    CHECK_INT_EQ(WM_FORWARD, fake_press(&world, desktop, &click));
    CHECK(world.focused == NULL);
    CHECK_INT_EQ(WM_CONSUMED, fake_key(desktop, FAKE_KEY_F4, WM_MOD_ALT));
    CHECK(world.closed == NULL);
    CHECK_INT_EQ(1, world.logout_calls);
    wm_desktop_destroy(desktop);
}

static void test_click_on_desktop_cancels_delayed_focus(void)
{
    fake_world world;
    wm_config config = fake_config();
    config.focus.click_to_focus = false;
    config.focus.focus_delay_ms = 100;
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *first = fake_map(desktop, 1, small);
    fake_map(desktop, 2, small);
    fake_click over = {WM_TARGET_CLIENT, first, {10, 10}, 0, 0, -1};
    fake_motion(&world, desktop, &over);
    fake_click click = {WM_TARGET_DESKTOP, NULL, {800, 500}, WM_MOUSE_LEFT, 0, -1};
    fake_press(&world, desktop, &click);
    fake_advance(&world, desktop, 100);
    CHECK(world.focused == NULL);
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
    fake_click where = {WM_TARGET_CLIENT, first, {10, 10}, 0, 0, -1};
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

static void test_zero_user_time_is_not_focused(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    fake_map(desktop, 1, small);
    wm_window_desc desc = fake_desc(2, small);
    desc.has_user_time = true;
    desc.user_time = 0;
    wm_window *second = wm_window_map(desktop, &desc);
    CHECK(wm_window_current_view(second).visible);
    CHECK(world.focused == fake_handle(1));
    CHECK_INT_EQ(1, fake_stack_index(&world, fake_handle(2)));
    wm_desktop_destroy(desktop);
}

static void test_old_user_time_cannot_steal_focus(void)
{
    fake_world world;
    wm_config config = fake_config();
    config.focus.prevent_focus_stealing = true;
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    fake_map(desktop, 1, small);
    wm_key_event key = {FAKE_KEY_D, 0, 5000};
    wm_key_pressed(desktop, &key);
    wm_window_desc old = fake_desc(2, small);
    old.has_user_time = true;
    old.user_time = 4000;
    wm_window_map(desktop, &old);
    CHECK(world.focused == fake_handle(1));
    wm_window_desc recent = fake_desc(3, small);
    recent.has_user_time = true;
    recent.user_time = 6000;
    wm_window_map(desktop, &recent);
    CHECK(world.focused == fake_handle(3));
    wm_desktop_destroy(desktop);
}

static void test_launched_window_ignores_old_user_time(void)
{
    fake_world world;
    wm_config config = fake_config();
    config.focus.prevent_focus_stealing = true;
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    fake_map(desktop, 1, small);
    wm_launch_started(desktop, "token-a");
    wm_key_event key = {FAKE_KEY_D, 0, 5000};
    wm_key_pressed(desktop, &key);
    wm_window_desc desc = fake_desc(2, small);
    desc.has_user_time = true;
    desc.user_time = 4000;
    desc.launch_id = "token-a";
    wm_window_map(desktop, &desc);
    CHECK(world.focused == fake_handle(2));
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
    fake_click click = {WM_TARGET_CLIENT, parent, {800, 500}, WM_MOUSE_LEFT, 0, -1};
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

static void test_activating_parent_keeps_dialog_focused(void)
{
    fake_world world;
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_start(&world, &config);
    wm_window *parent = fake_map(desktop, 1, small);
    fake_map_dialog(desktop, 2, parent);
    wm_request_activate(desktop, parent, WM_ACTIVATION_APPLICATION);
    CHECK(world.focused == fake_handle(2));
    CHECK(fake_stack_index(&world, fake_handle(2)) > fake_stack_index(&world, fake_handle(1)));
    wm_desktop_destroy(desktop);
}

static const check_case CASES[] = {
    {"new_window_gets_focus", test_new_window_gets_focus},
    {"zero_user_time_is_not_focused", test_zero_user_time_is_not_focused},
    {"old_user_time_cannot_steal_focus", test_old_user_time_cannot_steal_focus},
    {"launched_window_ignores_old_user_time", test_launched_window_ignores_old_user_time},
    {"activating_parent_keeps_dialog_focused", test_activating_parent_keeps_dialog_focused},
    {"click_on_client_focuses_and_raises", test_click_on_client_focuses_and_raises},
    {"click_on_desktop_clears_focus", test_click_on_desktop_clears_focus},
    {"click_on_desktop_cancels_delayed_focus", test_click_on_desktop_cancels_delayed_focus},
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

int main(void)
{
    return check_run(CASES, sizeof(CASES) / sizeof(CASES[0]));
}
