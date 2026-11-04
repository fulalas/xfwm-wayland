#include <string.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include "accelerator.h"
#include "check.h"
#include "keymap_store.h"
#include "shortcut_defaults.h"
#include "shortcut_property.h"

static size_t count_action(wm_keymap keymap, wm_action action)
{
    size_t count = 0;

    for (size_t i = 0; i < keymap.binding_count; i++) {
        count += keymap.bindings[i].action == action;
    }
    return count;
}

static void test_property_accelerator(void)
{
    const char *accelerator =
        shortcut_property_accelerator("/xfwm4/custom/<Alt>F4", "/xfwm4/custom");

    CHECK(accelerator && strcmp(accelerator, "<Alt>F4") == 0);
}

static void test_property_skips_children_and_other_bases(void)
{
    CHECK(!shortcut_property_accelerator("/commands/default/<Alt>F2/startup-notify",
                                         "/commands/default"));
    CHECK(!shortcut_property_accelerator("/xfwm4/default/Escape", "/xfwm4/custom"));
    CHECK(!shortcut_property_accelerator("/xfwm4/customer/a", "/xfwm4/custom"));
    CHECK(!shortcut_property_accelerator("/xfwm4/custom", "/xfwm4/custom"));
}

static void test_navigation_keys_added(void)
{
    keymap_store *store = keymap_store_create();
    wm_keymap keymap;

    keymap_store_ensure_navigation(store);
    keymap = keymap_store_view(store);
    CHECK_INT_EQ(5, keymap.binding_count);
    CHECK_INT_EQ(1, count_action(keymap, WM_ACTION_CANCEL));
    CHECK_INT_EQ(1, count_action(keymap, WM_ACTION_RIGHT));
    keymap_store_destroy(store);
}

static void test_navigation_not_duplicated(void)
{
    keymap_store *store = keymap_store_create();
    wm_key escape = { .keysym = XKB_KEY_Escape, .modifiers = 0 };

    keymap_store_add_action(store, escape, WM_ACTION_CANCEL);
    keymap_store_ensure_navigation(store);
    CHECK_INT_EQ(1, count_action(keymap_store_view(store), WM_ACTION_CANCEL));
    keymap_store_destroy(store);
}

static void test_unknown_action_ignored(void)
{
    keymap_store *store = keymap_store_create();
    wm_key key = { .keysym = XKB_KEY_a, .modifiers = WM_MOD_ALT };

    keymap_store_add_action(store, key, WM_ACTION_NONE);
    CHECK_INT_EQ(0, keymap_store_view(store).binding_count);
    keymap_store_destroy(store);
}

static void test_commands_are_copied(void)
{
    keymap_store *store = keymap_store_create();
    wm_key key = { .keysym = XKB_KEY_t, .modifiers = WM_MOD_CTRL | WM_MOD_ALT };
    char command[] = "xfce4-terminal";
    wm_keymap keymap;

    keymap_store_add_command(store, key, command);
    command[0] = 'X';
    keymap = keymap_store_view(store);
    CHECK_INT_EQ(1, keymap.command_count);
    CHECK(strcmp(keymap.commands[0].command, "xfce4-terminal") == 0);
    CHECK_INT_EQ(XKB_KEY_t, keymap.commands[0].key.keysym);
    keymap_store_destroy(store);
}

static void test_builtin_defaults_parse(void)
{
    size_t count;
    const shortcut_default *table = shortcut_defaults_table(&count);
    wm_key key;

    CHECK(count > 50);
    for (size_t i = 0; i < count; i++) {
        CHECK(accelerator_parse(table[i].accelerator, &key));
    }
}

int main(void)
{
    static const check_case cases[] = {
        { "property_accelerator", test_property_accelerator },
        { "property_skips_children_and_other_bases", test_property_skips_children_and_other_bases },
        { "navigation_keys_added", test_navigation_keys_added },
        { "navigation_not_duplicated", test_navigation_not_duplicated },
        { "unknown_action_ignored", test_unknown_action_ignored },
        { "commands_are_copied", test_commands_are_copied },
        { "builtin_defaults_parse", test_builtin_defaults_parse },
    };

    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
