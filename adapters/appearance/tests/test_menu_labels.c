#include <string.h>

#include <glib.h>

#include "appearance/appearance.h"
#include "check.h"
#include "menu_labels.h"

static bool text_is(const wm_menu_level *level, size_t index, const char *expected)
{
    char *text = appearance_menu_item_text(level, index);
    bool equal = strcmp(text, expected) == 0;

    g_free(text);
    return equal;
}

static void test_msgids_match_xfwm4(void)
{
    CHECK(strcmp(appearance_menu_label_msgid(WM_MENU_MAXIMIZE), "Ma_ximize") == 0);
    CHECK(strcmp(appearance_menu_label_msgid(WM_MENU_MINIMIZE_OTHERS),
                 "Minimize _Other Windows") == 0);
    CHECK(strcmp(appearance_menu_label_msgid(WM_MENU_NORMAL), "_Same as Other Windows") == 0);
    CHECK(strcmp(appearance_menu_label_msgid(WM_MENU_SHADE), "Roll Window Up") == 0);
    CHECK(strcmp(appearance_menu_label_msgid(WM_MENU_STICK), "Always on _Visible Workspace")
          == 0);
    CHECK(strcmp(appearance_menu_label_msgid(WM_MENU_MONITOR_DOWN), "Monitor Down") == 0);
    CHECK(strcmp(appearance_menu_label_msgid(WM_MENU_CLOSE), "_Close") == 0);
    CHECK(appearance_menu_label_msgid(WM_MENU_SEPARATOR) == NULL);
    CHECK(appearance_menu_label_msgid(WM_MENU_WORKSPACE_ENTRY) == NULL);
}

static void test_mnemonics_are_removed(void)
{
    int plain_at;
    int escaped_at;
    char *plain = appearance_strip_mnemonics("Ma_ximize", &plain_at);
    char *escaped = appearance_strip_mnemonics("a__b_", &escaped_at);

    CHECK(strcmp(plain, "Maximize") == 0);
    CHECK_INT_EQ(2, plain_at);
    CHECK(strcmp(escaped, "a_b_") == 0);
    CHECK_INT_EQ(-1, escaped_at);
    g_free(plain);
    g_free(escaped);
}

static void test_items_know_their_letter(void)
{
    wm_menu_level level = { .count = 2 };

    level.items[0] = (wm_menu_item) { .label = WM_MENU_CLOSE, .sensitive = true };
    level.items[1] = (wm_menu_item) { .label = WM_MENU_SHADE, .sensitive = true };
    CHECK_INT_EQ('c', appearance_menu_item_mnemonic(&level, 0));
    CHECK_INT_EQ(0, appearance_menu_item_mnemonic(&level, 1));
}

static void test_item_text_is_translated_label(void)
{
    wm_menu_level level = { .count = 2 };

    level.items[0] = (wm_menu_item) { .label = WM_MENU_UNFULLSCREEN, .sensitive = true };
    level.items[1] = (wm_menu_item) { .label = WM_MENU_WORKSPACES, .has_submenu = true };
    CHECK(text_is(&level, 0, "Leave Fullscreen"));
    CHECK(text_is(&level, 1, "Move to Another Workspace"));
}

static void test_workspace_entries(void)
{
    wm_menu_level level = { .count = 3 };

    level.items[0] = (wm_menu_item) { .label = WM_MENU_WORKSPACE_ENTRY };
    level.items[1] = (wm_menu_item) { .label = WM_MENU_WORKSPACE_ENTRY, .text = "Mail" };
    level.items[2] = (wm_menu_item) { .label = WM_MENU_WORKSPACE_ENTRY, .text = "a_b" };
    CHECK(text_is(&level, 0, "1"));
    CHECK(text_is(&level, 1, "2 (Mail)"));
    CHECK(text_is(&level, 2, "3 (a_b)"));
    CHECK_INT_EQ('2', appearance_menu_item_mnemonic(&level, 1));
    CHECK_INT_EQ(0, appearance_menu_item_mnemonic(&level, 2));
}

int main(void)
{
    static const check_case cases[] = {
        { "msgids_match_xfwm4", test_msgids_match_xfwm4 },
        { "mnemonics_are_removed", test_mnemonics_are_removed },
        { "items_know_their_letter", test_items_know_their_letter },
        { "item_text_is_translated_label", test_item_text_is_translated_label },
        { "workspace_entries", test_workspace_entries },
    };

    return check_run(cases, sizeof cases / sizeof cases[0]);
}
