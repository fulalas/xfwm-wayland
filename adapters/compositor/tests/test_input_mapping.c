#include <linux/input-event-codes.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/util/edges.h>
#include <xkbcommon/xkbcommon.h>

#include "check.h"
#include "input_mapping.h"
#include "wm/desktop.h"
#include "wm/keys.h"

static void test_modifiers_map_to_core_bits(void)
{
    uint32_t mods = input_mapping_modifiers(WLR_MODIFIER_CTRL | WLR_MODIFIER_ALT);
    CHECK_INT_EQ(WM_MOD_CTRL | WM_MOD_ALT, mods);
    CHECK_INT_EQ(WM_MOD_SUPER, input_mapping_modifiers(WLR_MODIFIER_LOGO));
    CHECK_INT_EQ(WM_MOD_SHIFT | WM_MOD_MOD5,
                 input_mapping_modifiers(WLR_MODIFIER_SHIFT | WLR_MODIFIER_MOD5));
}

static void test_lock_modifiers_are_ignored(void)
{
    uint32_t mods = input_mapping_modifiers(WLR_MODIFIER_CAPS | WLR_MODIFIER_MOD2);
    CHECK_INT_EQ(0, mods);
}

static void test_keysyms_are_lowercased(void)
{
    CHECK_INT_EQ(XKB_KEY_a, input_mapping_keysym(XKB_KEY_A));
    CHECK_INT_EQ(XKB_KEY_Tab, input_mapping_keysym(XKB_KEY_Tab));
}

static void test_vt_switch_from_translated_keysym(void)
{
    key_chord chord = {XKB_KEY_XF86Switch_VT_3, XKB_KEY_F3, WM_MOD_CTRL | WM_MOD_ALT};
    CHECK_INT_EQ(3, input_mapping_vt(&chord));
}

static void test_vt_switch_from_function_key(void)
{
    key_chord chord = {XKB_KEY_F12, XKB_KEY_F12, WM_MOD_CTRL | WM_MOD_ALT};
    CHECK_INT_EQ(12, input_mapping_vt(&chord));
}

static void test_function_key_without_chord_is_not_vt(void)
{
    key_chord alt_only = {XKB_KEY_F2, XKB_KEY_F2, WM_MOD_ALT};
    CHECK_INT_EQ(0, input_mapping_vt(&alt_only));
}

static void test_buttons_map_to_core_numbers(void)
{
    CHECK_INT_EQ(WM_MOUSE_LEFT, input_mapping_button(BTN_LEFT));
    CHECK_INT_EQ(WM_MOUSE_MIDDLE, input_mapping_button(BTN_MIDDLE));
    CHECK_INT_EQ(WM_MOUSE_RIGHT, input_mapping_button(BTN_RIGHT));
    CHECK_INT_EQ(WM_MOUSE_BACK, input_mapping_button(BTN_SIDE));
    CHECK_INT_EQ(WM_MOUSE_FORWARD, input_mapping_button(BTN_EXTRA));
    CHECK_INT_EQ(0, input_mapping_button(BTN_TASK));
}

static void test_wheel_clicks_become_steps(void)
{
    axis_steps steps = input_mapping_axis(0, 240);
    CHECK_INT_EQ(2, steps.steps);
    CHECK_INT_EQ(0, steps.remainder);
    steps = input_mapping_axis(0, -120);
    CHECK_INT_EQ(-1, steps.steps);
}

static void test_high_resolution_wheel_accumulates(void)
{
    axis_steps steps = input_mapping_axis(0, 60);
    CHECK_INT_EQ(0, steps.steps);
    steps = input_mapping_axis(steps.remainder, 60);
    CHECK_INT_EQ(1, steps.steps);
    CHECK_INT_EQ(0, steps.remainder);
}

static void test_edges_round_trip(void)
{
    uint32_t wlr = WLR_EDGE_TOP | WLR_EDGE_LEFT;
    uint32_t wm = input_mapping_edges_from_wlr(wlr);
    CHECK_INT_EQ(WM_EDGE_TOP | WM_EDGE_LEFT, wm);
    CHECK_INT_EQ(wlr, input_mapping_edges_to_wlr(wm));
}

int main(void)
{
    const check_case cases[] = {
        {"modifiers_map_to_core_bits", test_modifiers_map_to_core_bits},
        {"lock_modifiers_are_ignored", test_lock_modifiers_are_ignored},
        {"keysyms_are_lowercased", test_keysyms_are_lowercased},
        {"vt_switch_from_translated_keysym", test_vt_switch_from_translated_keysym},
        {"vt_switch_from_function_key", test_vt_switch_from_function_key},
        {"function_key_without_chord_is_not_vt", test_function_key_without_chord_is_not_vt},
        {"buttons_map_to_core_numbers", test_buttons_map_to_core_numbers},
        {"wheel_clicks_become_steps", test_wheel_clicks_become_steps},
        {"high_resolution_wheel_accumulates", test_high_resolution_wheel_accumulates},
        {"edges_round_trip", test_edges_round_trip},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
