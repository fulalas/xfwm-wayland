#include <string.h>

#include "check.h"
#include "fake_ports.h"

static wm_window_rule rule_for_app(const char *app_id, wm_rule_anchor anchor, int margin)
{
    wm_window_rule rule;
    memset(&rule, 0, sizeof(rule));
    rule.app_id = app_id;
    rule.anchor = anchor;
    rule.margin = margin;
    return rule;
}

static wm_desktop *desktop_with_panel(fake_world *world)
{
    fake_world_init(world);
    world->outputs[0].usable = (wm_rect) {0, 0, FAKE_SCREEN_WIDTH, FAKE_SCREEN_HEIGHT - 30};
    wm_config config = fake_config();
    return fake_desktop_create(world, &config);
}

static wm_rect frame_of(const wm_window *window)
{
    return wm_window_current_view(window).frame;
}

static void test_bottom_right_uses_work_area_and_margin(void)
{
    fake_world world;
    wm_desktop *desktop = desktop_with_panel(&world);
    wm_window_rule rule = rule_for_app("app", WM_ANCHOR_BOTTOM_RIGHT, 10);
    wm_desktop_set_window_rules(desktop, &rule, 1);
    wm_window *window = fake_map(desktop, 1, (wm_rect) {0, 0, 400, 300});
    wm_rect frame = frame_of(window);
    CHECK_INT_EQ(FAKE_SCREEN_WIDTH - 10, wm_rect_right(&frame));
    CHECK_INT_EQ(FAKE_SCREEN_HEIGHT - 30 - 10, wm_rect_bottom(&frame));
    CHECK_INT_EQ(400, wm_window_current_view(window).client.width);
    CHECK_INT_EQ(300, wm_window_current_view(window).client.height);
    wm_desktop_destroy(desktop);
}

static void test_top_and_center_anchors(void)
{
    fake_world world;
    wm_desktop *desktop = desktop_with_panel(&world);
    wm_window_rule rules[] = {
        rule_for_app("top", WM_ANCHOR_TOP, 5),
        rule_for_app("center", WM_ANCHOR_CENTER, 5),
        rule_for_app("left", WM_ANCHOR_TOP_LEFT, 0),
    };
    wm_desktop_set_window_rules(desktop, rules, 3);
    wm_window_desc desc = fake_desc(1, (wm_rect) {0, 0, 400, 300});
    desc.app_id = "top";
    wm_rect top = frame_of(wm_window_map(desktop, &desc));
    desc = fake_desc(2, (wm_rect) {0, 0, 400, 300});
    desc.app_id = "center";
    wm_rect center = frame_of(wm_window_map(desktop, &desc));
    desc = fake_desc(3, (wm_rect) {0, 0, 400, 300});
    desc.app_id = "left";
    wm_rect left = frame_of(wm_window_map(desktop, &desc));
    CHECK_INT_EQ(5, top.y);
    CHECK_INT_EQ((FAKE_SCREEN_WIDTH - top.width) / 2, top.x);
    CHECK_INT_EQ((FAKE_SCREEN_WIDTH - center.width) / 2, center.x);
    CHECK_INT_EQ((FAKE_SCREEN_HEIGHT - 30 - center.height) / 2, center.y);
    CHECK_INT_EQ(0, left.x);
    CHECK_INT_EQ(0, left.y);
    wm_desktop_destroy(desktop);
}

static void test_glob_and_exact_title(void)
{
    fake_world world;
    wm_desktop *desktop = desktop_with_panel(&world);
    wm_window_rule rules[] = {rule_for_app(NULL, WM_ANCHOR_TOP_LEFT, 0),
                              rule_for_app(NULL, WM_ANCHOR_BOTTOM_LEFT, 0)};
    rules[0].title = "Mouse*";
    rules[0].match = WM_MATCH_GLOB;
    rules[1].title = "Mouse";
    wm_desktop_set_window_rules(desktop, rules, 2);
    wm_window_desc desc = fake_desc(1, (wm_rect) {0, 0, 400, 300});
    desc.title = "Mousepad - notes";
    wm_rect frame = frame_of(wm_window_map(desktop, &desc));
    CHECK_INT_EQ(0, frame.x);
    CHECK_INT_EQ(0, frame.y);
    wm_desktop_destroy(desktop);
}

static void test_rule_without_match_fields_matches_nothing(void)
{
    fake_world world;
    wm_desktop *desktop = desktop_with_panel(&world);
    wm_window_rule rule = rule_for_app(NULL, WM_ANCHOR_TOP_LEFT, 0);
    wm_desktop_set_window_rules(desktop, &rule, 1);
    wm_window *window = fake_map(desktop, 1, (wm_rect) {0, 0, 400, 300});
    CHECK(frame_of(window).x != 0);
    wm_desktop_destroy(desktop);
}

static void test_changed_rule_moves_open_window(void)
{
    fake_world world;
    wm_desktop *desktop = desktop_with_panel(&world);
    wm_window *window = fake_map(desktop, 1, (wm_rect) {0, 0, 400, 300});
    wm_window_rule rule = rule_for_app("app", WM_ANCHOR_TOP_LEFT, 0);
    wm_desktop_set_window_rules(desktop, &rule, 1);
    CHECK_INT_EQ(0, frame_of(window).x);
    CHECK_INT_EQ(0, frame_of(window).y);
    rule.anchor = WM_ANCHOR_BOTTOM_RIGHT;
    wm_desktop_set_window_rules(desktop, &rule, 1);
    wm_rect frame = frame_of(window);
    CHECK_INT_EQ(FAKE_SCREEN_WIDTH, wm_rect_right(&frame));
    CHECK_INT_EQ(FAKE_SCREEN_HEIGHT - 30, wm_rect_bottom(&frame));
    wm_rect client = wm_window_current_view(window).client;
    CHECK_RECT_EQ(client, fake_configured(&world, fake_handle(1)));
    wm_desktop_destroy(desktop);
}

static void test_unchanged_rules_leave_moved_window(void)
{
    fake_world world;
    wm_desktop *desktop = desktop_with_panel(&world);
    wm_window_rule rules[] = {rule_for_app("app", WM_ANCHOR_TOP_LEFT, 0),
                              rule_for_app("other", WM_ANCHOR_CENTER, 0)};
    wm_desktop_set_window_rules(desktop, rules, 1);
    wm_window *window = fake_map(desktop, 1, (wm_rect) {0, 0, 400, 300});
    wm_rect moved = {300, 200, 400, 300};
    wm_request_configure(desktop, window, &moved);
    wm_rect before = wm_window_current_view(window).client;
    wm_desktop_set_window_rules(desktop, rules, 2);
    CHECK_RECT_EQ(before, wm_window_current_view(window).client);
    wm_desktop_destroy(desktop);
}

static void test_inserted_rule_leaves_other_windows(void)
{
    fake_world world;
    wm_desktop *desktop = desktop_with_panel(&world);
    wm_window_rule rules[] = {rule_for_app("other", WM_ANCHOR_CENTER, 0),
                              rule_for_app("app", WM_ANCHOR_TOP_LEFT, 0)};
    wm_desktop_set_window_rules(desktop, &rules[1], 1);
    wm_window *window = fake_map(desktop, 1, (wm_rect) {0, 0, 400, 300});
    wm_rect moved = {300, 200, 400, 300};
    wm_request_configure(desktop, window, &moved);
    wm_rect before = wm_window_current_view(window).client;
    wm_desktop_set_window_rules(desktop, rules, 2);
    CHECK_RECT_EQ(before, wm_window_current_view(window).client);
    wm_desktop_destroy(desktop);
}

static void test_window_that_never_takes_focus(void)
{
    fake_world world;
    wm_desktop *desktop = desktop_with_panel(&world);
    wm_window_rule rule = rule_for_app("app", WM_ANCHOR_TOP_RIGHT, 0);
    wm_desktop_set_window_rules(desktop, &rule, 1);
    wm_window_desc desc = fake_desc(1, (wm_rect) {0, 0, 400, 300});
    desc.capabilities &= ~(uint32_t) WM_CAP_FOCUS;
    wm_window *window = wm_window_map(desktop, &desc);
    CHECK(world.focused != fake_handle(1));
    rule.anchor = WM_ANCHOR_BOTTOM_LEFT;
    wm_desktop_set_window_rules(desktop, &rule, 1);
    wm_rect frame = frame_of(window);
    CHECK_INT_EQ(0, frame.x);
    CHECK_INT_EQ(FAKE_SCREEN_HEIGHT - 30, wm_rect_bottom(&frame));
    wm_desktop_destroy(desktop);
}

static void test_no_focus_rule(void)
{
    fake_world world;
    wm_desktop *desktop = desktop_with_panel(&world);
    fake_map(desktop, 1, (wm_rect) {0, 0, 400, 300});
    CHECK(world.focused == fake_handle(1));
    wm_window_rule rule = rule_for_app("quiet", WM_ANCHOR_NONE, 0);
    rule.no_focus = (wm_rule_switch) {true, true};
    wm_desktop_set_window_rules(desktop, &rule, 1);
    wm_window_desc desc = fake_desc(2, (wm_rect) {0, 0, 400, 300});
    desc.app_id = "quiet";
    wm_window *quiet = wm_window_map(desktop, &desc);
    CHECK(world.focused == fake_handle(1));
    rule.no_focus.on = false;
    wm_desktop_set_window_rules(desktop, &rule, 1);
    wm_request_activate(desktop, quiet, WM_ACTIVATION_PAGER);
    CHECK(world.focused == fake_handle(2));
    rule.no_focus.on = true;
    wm_desktop_set_window_rules(desktop, &rule, 1);
    CHECK(world.focused == fake_handle(1));
    wm_desktop_destroy(desktop);
}

static void test_output_by_name(void)
{
    fake_world world;
    fake_world_init(&world);
    wm_rect second = {FAKE_SCREEN_WIDTH, 0, 1280, 1024};
    fake_world_add_output(&world, second, second);
    world.outputs[0].name = "DP-1";
    world.outputs[1].name = "HDMI-A-1";
    wm_config config = fake_config();
    wm_desktop *desktop = fake_desktop_create(&world, &config);
    wm_window_rule rules[] = {rule_for_app("app", WM_ANCHOR_TOP_RIGHT, 0),
                              rule_for_app("plain", WM_ANCHOR_NONE, 0)};
    rules[0].output = "HDMI-A-1";
    rules[1].output = "HDMI-A-1";
    wm_desktop_set_window_rules(desktop, rules, 2);
    wm_window *anchored = fake_map(desktop, 1, (wm_rect) {0, 0, 400, 300});
    wm_rect frame = frame_of(anchored);
    CHECK_INT_EQ(wm_rect_right(&second), wm_rect_right(&frame));
    CHECK_INT_EQ(0, frame.y);
    wm_window_desc desc = fake_desc(2, (wm_rect) {0, 0, 400, 300});
    desc.app_id = "plain";
    wm_rect plain = frame_of(wm_window_map(desktop, &desc));
    CHECK(wm_rect_contains(&second, wm_rect_center(&plain)));
    rules[0].output = "DP-1";
    wm_desktop_set_window_rules(desktop, rules, 2);
    frame = frame_of(anchored);
    CHECK_INT_EQ(FAKE_SCREEN_WIDTH, wm_rect_right(&frame));
    wm_desktop_destroy(desktop);
}

static void test_workspace_above_and_skip_taskbar(void)
{
    fake_world world;
    wm_desktop *desktop = desktop_with_panel(&world);
    wm_window_rule rule = rule_for_app("app", WM_ANCHOR_NONE, 0);
    rule.has_workspace = true;
    rule.workspace = 2;
    rule.above = (wm_rule_switch) {true, true};
    rule.skip_taskbar = (wm_rule_switch) {true, true};
    wm_desktop_set_window_rules(desktop, &rule, 1);
    wm_window *window = fake_map(desktop, 1, (wm_rect) {0, 0, 400, 300});
    wm_window_view view = wm_window_current_view(window);
    CHECK_INT_EQ(2, view.workspace);
    CHECK(view.above);
    CHECK(view.skip_taskbar);
    rule.workspace = 1;
    rule.above.on = false;
    rule.skip_taskbar.on = false;
    wm_desktop_set_window_rules(desktop, &rule, 1);
    view = wm_window_current_view(window);
    CHECK_INT_EQ(1, view.workspace);
    CHECK(!view.above);
    CHECK(!view.skip_taskbar);
    wm_desktop_destroy(desktop);
}

static void test_maximized_window_keeps_its_place(void)
{
    fake_world world;
    wm_desktop *desktop = desktop_with_panel(&world);
    wm_window *window = fake_map(desktop, 1, (wm_rect) {0, 0, 400, 300});
    wm_state_change change = {WM_STATE_MAXIMIZED, 0};
    wm_request_state(desktop, window, &change);
    wm_rect maximized = frame_of(window);
    wm_window_rule rule = rule_for_app("app", WM_ANCHOR_BOTTOM_RIGHT, 0);
    wm_desktop_set_window_rules(desktop, &rule, 1);
    CHECK_RECT_EQ(maximized, frame_of(window));
    wm_desktop_destroy(desktop);
}

static void test_wide_window_stays_inside_left_edge(void)
{
    fake_world world;
    wm_desktop *desktop = desktop_with_panel(&world);
    wm_window_rule rule = rule_for_app("app", WM_ANCHOR_RIGHT, 20);
    wm_desktop_set_window_rules(desktop, &rule, 1);
    wm_window *window = fake_map(desktop, 1, (wm_rect) {0, 0, 2000, 300});
    CHECK_INT_EQ(0, frame_of(window).x);
    wm_desktop_destroy(desktop);
}

static const check_case CASES[] = {
    {"bottom_right_uses_work_area_and_margin", test_bottom_right_uses_work_area_and_margin},
    {"top_and_center_anchors", test_top_and_center_anchors},
    {"glob_and_exact_title", test_glob_and_exact_title},
    {"rule_without_match_fields_matches_nothing",
     test_rule_without_match_fields_matches_nothing},
    {"changed_rule_moves_open_window", test_changed_rule_moves_open_window},
    {"unchanged_rules_leave_moved_window", test_unchanged_rules_leave_moved_window},
    {"inserted_rule_leaves_other_windows", test_inserted_rule_leaves_other_windows},
    {"window_that_never_takes_focus", test_window_that_never_takes_focus},
    {"no_focus_rule", test_no_focus_rule},
    {"output_by_name", test_output_by_name},
    {"workspace_above_and_skip_taskbar", test_workspace_above_and_skip_taskbar},
    {"maximized_window_keeps_its_place", test_maximized_window_keeps_its_place},
    {"wide_window_stays_inside_left_edge", test_wide_window_stays_inside_left_edge},
};

int main(void)
{
    return check_run(CASES, sizeof(CASES) / sizeof(CASES[0]));
}
