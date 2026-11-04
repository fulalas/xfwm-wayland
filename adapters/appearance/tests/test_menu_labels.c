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
    CHECK(strcmp(appearance_menu_label_msgid(WM_MENU_MONITOR_DOWN), "Monitor _Down") == 0);
    CHECK(strcmp(appearance_menu_label_msgid(WM_MENU_CLOSE), "_Close") == 0);
    CHECK(appearance_menu_label_msgid(WM_MENU_SEPARATOR) == NULL);
    CHECK(appearance_menu_label_msgid(WM_MENU_WORKSPACE_ENTRY) == NULL);
}

static void test_mnemonics_are_removed(void)
{
    char *plain = appearance_strip_mnemonics("Ma_ximize");
    char *escaped = appearance_strip_mnemonics("a__b_");

    CHECK(strcmp(plain, "Maximize") == 0);
    CHECK(strcmp(escaped, "a_b_") == 0);
    g_free(plain);
    g_free(escaped);
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
    level.items[1] = (wm_menu_item) { .label = WM_MENU_WORKSPACE_ENTRY, .text = "_2 (Mail)" };
    level.items[2] = (wm_menu_item) { .label = WM_MENU_WORKSPACE_ENTRY };
    CHECK(text_is(&level, 0, "Workspace 1"));
    CHECK(text_is(&level, 1, "2 (Mail)"));
    CHECK(text_is(&level, 2, "Workspace 3"));
}

int main(void)
{
    static const check_case cases[] = {
        { "msgids_match_xfwm4", test_msgids_match_xfwm4 },
        { "mnemonics_are_removed", test_mnemonics_are_removed },
        { "item_text_is_translated_label", test_item_text_is_translated_label },
        { "workspace_entries", test_workspace_entries },
    };

    return check_run(cases, sizeof cases / sizeof cases[0]);
}
