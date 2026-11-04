#include <xkbcommon/xkbcommon-keysyms.h>

#include "accelerator.h"
#include "check.h"

static wm_key parse_ok(const char *text)
{
    wm_key key = { 0 };

    CHECK(accelerator_parse(text, &key));
    return key;
}

static void test_primary_alt_left(void)
{
    wm_key key = parse_ok("<Primary><Alt>Left");

    CHECK_INT_EQ(XKB_KEY_Left, key.keysym);
    CHECK_INT_EQ(WM_MOD_CTRL | WM_MOD_ALT, key.modifiers);
}

static void test_super_keypad(void)
{
    wm_key key = parse_ok("<Super>KP_Left");

    CHECK_INT_EQ(XKB_KEY_KP_Left, key.keysym);
    CHECK_INT_EQ(WM_MOD_SUPER, key.modifiers);
}

static void test_alt_function_key(void)
{
    wm_key key = parse_ok("<Alt>F4");

    CHECK_INT_EQ(XKB_KEY_F4, key.keysym);
    CHECK_INT_EQ(WM_MOD_ALT, key.modifiers);
}

static void test_plain_key(void)
{
    wm_key key = parse_ok("Escape");

    CHECK_INT_EQ(XKB_KEY_Escape, key.keysym);
    CHECK_INT_EQ(0, key.modifiers);
}

static void test_shift_page_down(void)
{
    wm_key key = parse_ok("<Shift><Alt>Page_Down");

    CHECK_INT_EQ(XKB_KEY_Page_Down, key.keysym);
    CHECK_INT_EQ(WM_MOD_SHIFT | WM_MOD_ALT, key.modifiers);
}

static void test_keysym_is_lowercased(void)
{
    CHECK_INT_EQ(XKB_KEY_l, parse_ok("<Super>L").keysym);
}

static void test_key_name_case_insensitive(void)
{
    CHECK_INT_EQ(XKB_KEY_Escape, parse_ok("<Alt>escape").keysym);
}

static void test_modifier_names_case_insensitive(void)
{
    CHECK_INT_EQ(WM_MOD_CTRL | WM_MOD_SHIFT, parse_ok("<control><SHIFT>a").modifiers);
}

static void test_modifier_aliases(void)
{
    CHECK_INT_EQ(WM_MOD_CTRL, parse_ok("<Ctrl>a").modifiers);
    CHECK_INT_EQ(WM_MOD_ALT, parse_ok("<Mod1>a").modifiers);
    CHECK_INT_EQ(WM_MOD_SUPER, parse_ok("<Mod4>a").modifiers);
    CHECK_INT_EQ(WM_MOD_SUPER, parse_ok("<Hyper>a").modifiers);
    CHECK_INT_EQ(WM_MOD_SUPER, parse_ok("<Meta>a").modifiers);
    CHECK_INT_EQ(WM_MOD_MOD3, parse_ok("<Mod3>a").modifiers);
    CHECK_INT_EQ(WM_MOD_MOD5, parse_ok("<Mod5>a").modifiers);
}

static void test_unknown_modifier_is_skipped(void)
{
    wm_key key = parse_ok("<Release><Alt>a");

    CHECK_INT_EQ(WM_MOD_ALT, key.modifiers);
    CHECK_INT_EQ(XKB_KEY_a, key.keysym);
}

static void test_rejects_invalid(void)
{
    wm_key key;

    CHECK(!accelerator_parse("<Control>", &key));
    CHECK(!accelerator_parse("<Alt>NoSuchKeyName", &key));
    CHECK(!accelerator_parse("<Alt", &key));
    CHECK(!accelerator_parse("none", &key));
    CHECK(!accelerator_parse("", &key));
    CHECK(!accelerator_parse(NULL, &key));
}

static void test_modifier_from_name(void)
{
    CHECK_INT_EQ(WM_MOD_ALT, accelerator_modifier_from_name("Alt"));
    CHECK_INT_EQ(WM_MOD_SUPER, accelerator_modifier_from_name("super"));
    CHECK_INT_EQ(0, accelerator_modifier_from_name("Bogus"));
}

int main(void)
{
    static const check_case cases[] = {
        { "primary_alt_left", test_primary_alt_left },
        { "super_keypad", test_super_keypad },
        { "alt_function_key", test_alt_function_key },
        { "plain_key", test_plain_key },
        { "shift_page_down", test_shift_page_down },
        { "keysym_is_lowercased", test_keysym_is_lowercased },
        { "key_name_case_insensitive", test_key_name_case_insensitive },
        { "modifier_names_case_insensitive", test_modifier_names_case_insensitive },
        { "modifier_aliases", test_modifier_aliases },
        { "unknown_modifier_is_skipped", test_unknown_modifier_is_skipped },
        { "rejects_invalid", test_rejects_invalid },
        { "modifier_from_name", test_modifier_from_name },
    };

    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
