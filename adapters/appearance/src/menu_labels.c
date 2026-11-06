#include "menu_labels.h"

#include <string.h>

#include <glib.h>
#include <glib/gi18n.h>

static const char xfwm4_domain[] = "xfwm4";
static const char own_domain[] = APPEARANCE_TEXT_DOMAIN;
enum {
    WORKSPACE_MNEMONIC_LAST = 9,
};

typedef struct label_msgid {
    wm_menu_label label;
    const char *msgid;
} label_msgid;

static const label_msgid label_msgids[] = {
    { WM_MENU_MAXIMIZE, "Ma_ximize" },
    { WM_MENU_UNMAXIMIZE, "Unma_ximize" },
    { WM_MENU_MINIMIZE, "Mi_nimize" },
    { WM_MENU_MINIMIZE_OTHERS, "Minimize _Other Windows" },
    { WM_MENU_SHOW, "S_how" },
    { WM_MENU_MOVE, "_Move" },
    { WM_MENU_RESIZE, "_Resize" },
    { WM_MENU_ABOVE, "Always on _Top" },
    { WM_MENU_NORMAL, "_Same as Other Windows" },
    { WM_MENU_BELOW, "Always _Below Other Windows" },
    { WM_MENU_SHADE, "Roll Window Up" },
    { WM_MENU_UNSHADE, "Roll Window Down" },
    { WM_MENU_FULLSCREEN, "_Fullscreen" },
    { WM_MENU_UNFULLSCREEN, "Leave _Fullscreen" },
    { WM_MENU_STICK, "Always on _Visible Workspace" },
    { WM_MENU_WORKSPACES, "Move to Another _Workspace" },
    { WM_MENU_MONITORS, "Move to Another Monitor" },
    { WM_MENU_MONITOR_LEFT, N_("Monitor Left") },
    { WM_MENU_MONITOR_RIGHT, N_("Monitor Right") },
    { WM_MENU_MONITOR_UP, N_("Monitor Up") },
    { WM_MENU_MONITOR_DOWN, N_("Monitor Down") },
    { WM_MENU_CLOSE, "_Close" },
};

static char *workspace_entry_label(const wm_menu_level *level, size_t index);
static char *item_label(const wm_menu_level *level, size_t index);
static int workspace_ordinal(const wm_menu_level *level, size_t index);
static const char *translate(const char *msgid);

const char *appearance_menu_label_msgid(wm_menu_label label)
{
    for (size_t i = 0; i < G_N_ELEMENTS(label_msgids); i++) {
        if (label_msgids[i].label == label) {
            return label_msgids[i].msgid;
        }
    }
    return NULL;
}

char *appearance_menu_item_text(const wm_menu_level *level, size_t index)
{
    return appearance_menu_item_text_with_mnemonic(level, index, NULL);
}

char *appearance_menu_item_text_with_mnemonic(const wm_menu_level *level, size_t index,
                                              int *mnemonic_at)
{
    char *label = item_label(level, index);
    char *text = appearance_strip_mnemonics(label, mnemonic_at);
    g_free(label);
    return text;
}

uint32_t appearance_menu_item_mnemonic(const wm_menu_level *level, size_t index)
{
    int mnemonic_at;
    char *text = appearance_menu_item_text_with_mnemonic(level, index, &mnemonic_at);
    uint32_t mnemonic = mnemonic_at >= 0 ? g_unichar_tolower(g_utf8_get_char(text + mnemonic_at))
                                         : 0;
    g_free(text);
    return mnemonic;
}

char *appearance_strip_mnemonics(const char *label, int *mnemonic_at)
{
    GString *text = g_string_sized_new(strlen(label));
    int found = -1;

    for (const char *c = label; *c != '\0'; c++) {
        if (*c == '_' && c[1] != '\0') {
            c++;
            if (*c != '_' && found < 0) {
                found = (int) text->len;
            }
        }
        g_string_append_c(text, *c);
    }
    if (mnemonic_at != NULL) {
        *mnemonic_at = found;
    }
    return g_string_free(text, FALSE);
}

static char *item_label(const wm_menu_level *level, size_t index)
{
    const wm_menu_item *item = &level->items[index];

    if (item->label == WM_MENU_WORKSPACE_ENTRY) {
        return workspace_entry_label(level, index);
    }
    const char *msgid = appearance_menu_label_msgid(item->label);
    if (msgid == NULL) {
        return g_strdup(item->text != NULL ? item->text : "");
    }
    return g_strdup(translate(msgid));
}

static char *workspace_entry_label(const wm_menu_level *level, size_t index)
{
    const char *name = level->items[index].text;
    int number = workspace_ordinal(level, index);
    const char *mnemonic = number <= WORKSPACE_MNEMONIC_LAST ? "_" : "";

    if (name == NULL || *name == '\0') {
        return g_strdup_printf("%s%d", mnemonic, number);
    }
    char **parts = g_strsplit(name, "_", -1);
    char *escaped = g_strjoinv("__", parts);
    g_strfreev(parts);
    char *label = g_strdup_printf("%s%d (%s)", strchr(name, '_') == NULL ? mnemonic : "", number,
                                  escaped);
    g_free(escaped);
    return label;
}

static int workspace_ordinal(const wm_menu_level *level, size_t index)
{
    int ordinal = 0;

    for (size_t i = 0; i <= index; i++) {
        if (level->items[i].label == WM_MENU_WORKSPACE_ENTRY) {
            ordinal++;
        }
    }
    return ordinal;
}

static const char *translate(const char *msgid)
{
    const char *text = dgettext(xfwm4_domain, msgid);
    return text != msgid ? text : dgettext(own_domain, msgid);
}
