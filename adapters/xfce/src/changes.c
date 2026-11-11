#include "changes.h"

#include <glib.h>
#include <stdbool.h>
#include <string.h>

#include "window_rules.h"

static const char *const channel_names[CHANNEL_COUNT] = {
    [CHANNEL_XFWM4] = "xfwm4",
    [CHANNEL_XSETTINGS] = "xsettings",
    [CHANNEL_SHORTCUTS] = "xfce4-keyboard-shortcuts",
    [CHANNEL_KEYBOARD_LAYOUT] = "keyboard-layout",
    [CHANNEL_KEYBOARDS] = "keyboards",
    [CHANNEL_POINTERS] = "pointers",
    [CHANNEL_DISPLAYS] = "displays-wl",
    [CHANNEL_ACCESSIBILITY] = "accessibility",
    [CHANNEL_POWER_MANAGER] = "xfce4-power-manager",
    [CHANNEL_PANEL] = "xfce4-panel",
};

static const char *const theme_properties[] = { "/general/theme", "/general/title_font" };

static unsigned xfwm4_flags(const char *property);
static bool is_theme_property(const char *property);
static unsigned xsettings_flags(const char *property);
static unsigned shortcut_flags(const char *property);
static unsigned power_flags(const char *property);
static unsigned panel_flags(const char *property);

const char *channel_name(channel_id id)
{
    return channel_names[id];
}

unsigned change_flags_for(channel_id id, const char *property)
{
    switch (id) {
    case CHANNEL_XFWM4:
        return xfwm4_flags(property);
    case CHANNEL_XSETTINGS:
        return xsettings_flags(property);
    case CHANNEL_SHORTCUTS:
        return shortcut_flags(property);
    case CHANNEL_KEYBOARD_LAYOUT:
    case CHANNEL_KEYBOARDS:
    case CHANNEL_POINTERS:
        return CHANGE_INPUT;
    case CHANNEL_DISPLAYS:
        return CHANGE_DISPLAYS;
    case CHANNEL_POWER_MANAGER:
        return power_flags(property);
    case CHANNEL_PANEL:
        return panel_flags(property);
    default:
        return 0;
    }
}

static unsigned xfwm4_flags(const char *property)
{
    if (is_theme_property(property)) {
        return CHANGE_THEME;
    }
    size_t root_length = strlen(window_rules_root);
    if (strncmp(property, window_rules_root, root_length) == 0 && property[root_length] == '/') {
        return CHANGE_WINDOW_RULES;
    }
    return g_str_has_prefix(property, "/general/") ? CHANGE_SETTINGS : 0;
}

static bool is_theme_property(const char *property)
{
    for (size_t i = 0; i < G_N_ELEMENTS(theme_properties); i++) {
        if (strcmp(property, theme_properties[i]) == 0) {
            return true;
        }
    }
    return false;
}

static unsigned xsettings_flags(const char *property)
{
    unsigned flags = CHANGE_X11_SETTINGS;
    if (g_str_has_prefix(property, "/Net/DoubleClick")) {
        flags |= CHANGE_SETTINGS;
    }
    if (g_str_has_prefix(property, "/Gtk/CursorTheme")) {
        flags |= CHANGE_INPUT;
    }
    return flags;
}

static unsigned shortcut_flags(const char *property)
{
    if (g_str_has_prefix(property, "/xfwm4/") || g_str_has_prefix(property, "/commands/")) {
        return CHANGE_SHORTCUTS;
    }
    return 0;
}

static unsigned power_flags(const char *property)
{
    if (g_str_has_prefix(property, "/xfce4-power-manager/dpms")
        || strcmp(property, "/xfce4-power-manager/presentation-mode") == 0) {
        return CHANGE_POWER;
    }
    return 0;
}

static unsigned panel_flags(const char *property)
{
    if (!g_str_has_prefix(property, "/plugins/plugin-")) {
        return 0;
    }
    const char *rest = strchr(property + strlen("/plugins/"), '/');
    return rest == NULL || strcmp(rest, "/rows") == 0 ? CHANGE_SETTINGS : 0;
}
