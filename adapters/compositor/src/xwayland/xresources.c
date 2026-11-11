#include "xresources.h"

#include <ctype.h>
#include <glib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

enum {
    XFT_DEFAULT = -1,
    VALUE_MAX = 256,
};

typedef struct resource_rule {
    const char *key;
    const char *setting;
} resource_rule;

static const char theme_core_key[] = "Xcursor.theme_core";

static const resource_rule rules[] = {
    {"Xft.antialias", "Xft/Antialias"},
    {"Xft.hinting", "Xft/Hinting"},
    {"Xft.hintstyle", "Xft/HintStyle"},
    {"Xft.rgba", "Xft/RGBA"},
    {"Xft.lcdfilter", "Xft/Lcdfilter"},
    {"Xft.dpi", X11_SETTING_XFT_DPI},
    {"Xcursor.theme", "Gtk/CursorThemeName"},
    {"Xcursor.size", xsettings_cursor_size},
};

static void keep_foreign_lines(GString *out, const char *existing);
static bool is_managed_line(const char *line, size_t length);
static bool line_has_key(const char *line, size_t length, const char *key);
static void append_rule(GString *out, const resource_rule *rule, const xsettings_table *settings);
static bool format_value(const x11_setting *setting, char *value, size_t capacity);
static void append_entry(GString *out, const char *key, const char *value);

char *xresources_merge(const char *existing, const xsettings_table *settings)
{
    GString *out = g_string_new(NULL);
    keep_foreign_lines(out, existing != NULL ? existing : "");
    for (size_t i = 0; i < sizeof(rules) / sizeof(rules[0]); i++) {
        append_rule(out, &rules[i], settings);
    }
    append_entry(out, theme_core_key, "1");
    return g_string_free(out, FALSE);
}

static void keep_foreign_lines(GString *out, const char *existing)
{
    const char *line = existing;
    while (*line != '\0') {
        const char *end = strchr(line, '\n');
        size_t length = end != NULL ? (size_t) (end - line) : strlen(line);
        if (length > 0 && !is_managed_line(line, length)) {
            g_string_append_len(out, line, (gssize) length);
            g_string_append_c(out, '\n');
        }
        line += end != NULL ? length + 1 : length;
    }
}

static bool is_managed_line(const char *line, size_t length)
{
    for (size_t i = 0; i < sizeof(rules) / sizeof(rules[0]); i++) {
        if (line_has_key(line, length, rules[i].key)) {
            return true;
        }
    }
    return line_has_key(line, length, theme_core_key);
}

static bool line_has_key(const char *line, size_t length, const char *key)
{
    size_t key_length = strlen(key);
    if (length <= key_length || strncmp(line, key, key_length) != 0) {
        return false;
    }
    size_t i = key_length;
    while (i < length && isspace((unsigned char) line[i])) {
        i++;
    }
    return i < length && line[i] == ':';
}

static void append_rule(GString *out, const resource_rule *rule, const xsettings_table *settings)
{
    const x11_setting *setting = xsettings_table_find(settings, rule->setting);
    x11_setting unscaled;
    if (setting != NULL && strcmp(rule->setting, xsettings_cursor_size) == 0) {
        unscaled = *setting;
        unscaled.integer = xsettings_table_cursor_size(settings);
        setting = &unscaled;
    }
    char value[VALUE_MAX];
    if (setting != NULL && format_value(setting, value, sizeof(value))) {
        append_entry(out, rule->key, value);
    }
}

static bool format_value(const x11_setting *setting, char *value, size_t capacity)
{
    if (setting->type == X11_SETTING_STRING) {
        snprintf(value, capacity, "%s", setting->string);
        return setting->string[0] != '\0';
    }
    if (setting->integer == XFT_DEFAULT) {
        return false;
    }
    bool is_dpi = strcmp(setting->name, X11_SETTING_XFT_DPI) == 0;
    int number = is_dpi ? setting->integer / X11_SETTING_XFT_DPI_UNIT : setting->integer;
    snprintf(value, capacity, "%d", number);
    return true;
}

static void append_entry(GString *out, const char *key, const char *value)
{
    g_string_append_printf(out, "%s:\t%s\n", key, value);
}
