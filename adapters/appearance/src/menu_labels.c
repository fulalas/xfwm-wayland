#include "menu_labels.h"

#include <string.h>

#include <glib.h>
#include <glib/gi18n.h>

static const char xfwm4_domain[] = "xfwm4";
static const char own_domain[] = "xfwm-wayland";
static const char workspace_format[] = "Workspace %d";

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

static char *workspace_entry_text(const wm_menu_level *level, size_t index);
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
    const wm_menu_item *item = &level->items[index];

    if (item->label == WM_MENU_WORKSPACE_ENTRY) {
        return workspace_entry_text(level, index);
    }
    const char *msgid = appearance_menu_label_msgid(item->label);
    if (msgid == NULL) {
        return g_strdup(item->text != NULL ? item->text : "");
    }
    return appearance_strip_mnemonics(translate(msgid));
}

char *appearance_strip_mnemonics(const char *label)
{
    GString *text = g_string_sized_new(strlen(label));

    for (const char *c = label; *c != '\0'; c++) {
        if (*c == '_' && c[1] != '\0') {
            c++;
        }
        g_string_append_c(text, *c);
    }
    return g_string_free(text, FALSE);
}

static char *workspace_entry_text(const wm_menu_level *level, size_t index)
{
    const char *text = level->items[index].text;

    if (text != NULL) {
        return appearance_strip_mnemonics(text);
    }
    return g_strdup_printf(dgettext(xfwm4_domain, workspace_format),
                           workspace_ordinal(level, index));
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
