#include "check.h"
#include "output/display_policy.h"

enum {
    MAX_OUTPUTS = 3,
};

static void test_internal_names(void)
{
    CHECK(display_is_internal("eDP-1"));
    CHECK(display_is_internal("LVDS-1"));
    CHECK(display_is_internal("DSI-1"));
    CHECK(!display_is_internal("HDMI-A-1"));
    CHECK(!display_is_internal("DP-1"));
    CHECK(!display_is_internal(NULL));
}

static void test_open_lid_keeps_wishes(void)
{
    const display_wish wishes[] = {{true, true}, {false, true}};
    bool enabled[MAX_OUTPUTS] = {false};
    display_policy_decide(&(display_wishes){wishes, 2, false}, enabled);
    CHECK(enabled[0] && enabled[1]);
}

static void test_closed_lid_turns_panel_off_with_external(void)
{
    const display_wish wishes[] = {{true, true}, {false, true}};
    bool enabled[MAX_OUTPUTS] = {false};
    display_policy_decide(&(display_wishes){wishes, 2, true}, enabled);
    CHECK(!enabled[0] && enabled[1]);
}

static void test_closed_lid_keeps_panel_when_alone(void)
{
    const display_wish wishes[] = {{true, true}};
    bool enabled[MAX_OUTPUTS] = {false};
    display_policy_decide(&(display_wishes){wishes, 1, true}, enabled);
    CHECK(enabled[0]);
}

static void test_closed_lid_keeps_panel_when_external_is_off(void)
{
    const display_wish wishes[] = {{true, true}, {false, false}};
    bool enabled[MAX_OUTPUTS] = {false};
    display_policy_decide(&(display_wishes){wishes, 2, true}, enabled);
    CHECK(enabled[0] && !enabled[1]);
}

static void test_never_leaves_every_output_off(void)
{
    const display_wish wishes[] = {{false, false}, {true, false}};
    bool open[MAX_OUTPUTS] = {false};
    bool closed[MAX_OUTPUTS] = {false};
    display_policy_decide(&(display_wishes){wishes, 2, false}, open);
    display_policy_decide(&(display_wishes){wishes, 2, true}, closed);
    CHECK(!open[0] && open[1]);
    CHECK(closed[0] && !closed[1]);
}

static void test_no_outputs(void)
{
    bool enabled[MAX_OUTPUTS] = {false};
    display_policy_decide(&(display_wishes){NULL, 0, true}, enabled);
    CHECK(!enabled[0]);
}

int main(void)
{
    const check_case cases[] = {
        {"internal_names", test_internal_names},
        {"open_lid_keeps_wishes", test_open_lid_keeps_wishes},
        {"closed_lid_turns_panel_off_with_external",
         test_closed_lid_turns_panel_off_with_external},
        {"closed_lid_keeps_panel_when_alone", test_closed_lid_keeps_panel_when_alone},
        {"closed_lid_keeps_panel_when_external_is_off",
         test_closed_lid_keeps_panel_when_external_is_off},
        {"never_leaves_every_output_off", test_never_leaves_every_output_off},
        {"no_outputs", test_no_outputs},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
