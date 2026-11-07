#include "app_shortcuts.h"

#include <string.h>
#include <xkbcommon/xkbcommon.h>

#include "accelerator.h"
#include "check.h"

static void test_trigger_reads_portal_names(void)
{
    wm_key key;

    CHECK(accelerator_parse_trigger("CTRL+ALT+t", &key));
    CHECK_INT_EQ(XKB_KEY_t, key.keysym);
    CHECK_INT_EQ(WM_MOD_CTRL | WM_MOD_ALT, key.modifiers);
    CHECK(accelerator_parse_trigger("LOGO+SHIFT+F12", &key));
    CHECK_INT_EQ(XKB_KEY_F12, key.keysym);
    CHECK_INT_EQ(WM_MOD_SUPER | WM_MOD_SHIFT, key.modifiers);
    CHECK(accelerator_parse_trigger("XF86AudioRaiseVolume", &key));
    CHECK_INT_EQ(XKB_KEY_XF86AudioRaiseVolume, key.keysym);
    CHECK_INT_EQ(0, key.modifiers);
    CHECK(accelerator_parse_trigger("NUM+CTRL+KP_1", &key));
    CHECK_INT_EQ(WM_MOD_CTRL, key.modifiers);
}

static void test_trigger_rejects_bad_text(void)
{
    wm_key key;

    CHECK(!accelerator_parse_trigger("", &key));
    CHECK(!accelerator_parse_trigger(NULL, &key));
    CHECK(!accelerator_parse_trigger("CTRL+", &key));
    CHECK(!accelerator_parse_trigger("BOGUS+a", &key));
    CHECK(!accelerator_parse_trigger("NoSuchKeyName", &key));
}

static void test_bound_shortcuts_enter_the_keymap(void)
{
    app_shortcuts *shortcuts = app_shortcuts_create();
    app_shortcut_request requests[] = {
        { "up", "Volume up", "XF86AudioRaiseVolume" },
        { "menu", "Open menu", NULL },
    };
    const wm_app_shortcut *keymap = NULL;

    app_shortcuts_bind(shortcuts, "/s/1", (app_shortcut_requests) { requests, 2 });
    CHECK_INT_EQ(2, app_shortcuts_count(shortcuts));
    CHECK_INT_EQ(1, app_shortcuts_keymap(shortcuts, &keymap));
    const app_shortcut_entry *entry = app_shortcuts_find(shortcuts, keymap[0].id);
    CHECK(entry != NULL && strcmp(entry->shortcut_id, "up") == 0);
    CHECK(!app_shortcuts_at(shortcuts, 1)->bound);
    app_shortcuts_destroy(shortcuts);
}

static void test_taken_key_passes_on_when_its_session_ends(void)
{
    app_shortcuts *shortcuts = app_shortcuts_create();
    app_shortcut_request first[] = { { "up", "Up", "XF86AudioRaiseVolume" } };
    app_shortcut_request second[] = { { "louder", "Louder", "XF86AudioRaiseVolume" } };
    const wm_app_shortcut *keymap = NULL;

    app_shortcuts_bind(shortcuts, "/s/1", (app_shortcut_requests) { first, 1 });
    app_shortcuts_bind(shortcuts, "/s/2", (app_shortcut_requests) { second, 1 });
    CHECK_INT_EQ(1, app_shortcuts_keymap(shortcuts, &keymap));
    CHECK(strcmp(app_shortcuts_find(shortcuts, keymap[0].id)->session, "/s/1") == 0);
    CHECK(app_shortcuts_drop_session(shortcuts, "/s/1"));
    CHECK_INT_EQ(1, app_shortcuts_keymap(shortcuts, &keymap));
    CHECK(strcmp(app_shortcuts_find(shortcuts, keymap[0].id)->session, "/s/2") == 0);
    CHECK(!app_shortcuts_drop_session(shortcuts, "/s/1"));
    app_shortcuts_destroy(shortcuts);
}

static void test_typing_keys_are_refused(void)
{
    app_shortcuts *shortcuts = app_shortcuts_create();
    app_shortcut_request requests[] = { { "a", "A", "a" }, { "enter", "Enter", "Return" } };
    const wm_app_shortcut *keymap = NULL;

    app_shortcuts_bind(shortcuts, "/s/1", (app_shortcut_requests) { requests, 2 });
    CHECK_INT_EQ(0, app_shortcuts_keymap(shortcuts, &keymap));
    app_shortcuts_destroy(shortcuts);
}

int main(void)
{
    static const check_case cases[] = {
        { "trigger_reads_portal_names", test_trigger_reads_portal_names },
        { "trigger_rejects_bad_text", test_trigger_rejects_bad_text },
        { "bound_shortcuts_enter_the_keymap", test_bound_shortcuts_enter_the_keymap },
        { "taken_key_passes_on_when_its_session_ends",
          test_taken_key_passes_on_when_its_session_ends },
        { "typing_keys_are_refused", test_typing_keys_are_refused },
    };

    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
