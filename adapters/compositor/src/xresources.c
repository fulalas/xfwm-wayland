#include "xresources.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    XFT_DPI_UNIT = 1024,
    XFT_DEFAULT = -1,
    VALUE_MAX = 256,
};

typedef struct resource_rule {
    const char *key;
    const char *setting;
} resource_rule;

typedef struct text {
    char *data;
    size_t length;
    size_t capacity;
} text;

static const char xft_dpi_setting[] = "Xft/DPI";
static const char theme_core_key[] = "Xcursor.theme_core";

static const resource_rule rules[] = {
    {"Xft.antialias", "Xft/Antialias"},
    {"Xft.hinting", "Xft/Hinting"},
    {"Xft.hintstyle", "Xft/HintStyle"},
    {"Xft.rgba", "Xft/RGBA"},
    {"Xft.lcdfilter", "Xft/Lcdfilter"},
    {"Xft.dpi", xft_dpi_setting},
    {"Xcursor.theme", "Gtk/CursorThemeName"},
    {"Xcursor.size", "Gtk/CursorThemeSize"},
};

static void keep_foreign_lines(text *out, const char *existing);
static bool is_managed_line(const char *line, size_t length);
static bool line_has_key(const char *line, size_t length, const char *key);
static void append_rule(text *out, const resource_rule *rule, const xsettings_table *settings);
static bool format_value(const x11_setting *setting, char *value, size_t capacity);
static void append_entry(text *out, const char *key, const char *value);
static void append(text *out, const char *bytes, size_t length);

char *xresources_merge(const char *existing, const xsettings_table *settings)
{
    text out = {NULL, 0, 0};
    append(&out, "", 0);
    keep_foreign_lines(&out, existing != NULL ? existing : "");
    for (size_t i = 0; i < sizeof(rules) / sizeof(rules[0]); i++) {
        append_rule(&out, &rules[i], settings);
    }
    append_entry(&out, theme_core_key, "1");
    return out.data;
}

static void keep_foreign_lines(text *out, const char *existing)
{
    const char *line = existing;
    while (*line != '\0') {
        const char *end = strchr(line, '\n');
        size_t length = end != NULL ? (size_t) (end - line) : strlen(line);
        if (length > 0 && !is_managed_line(line, length)) {
            append(out, line, length);
            append(out, "\n", 1);
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

static void append_rule(text *out, const resource_rule *rule, const xsettings_table *settings)
{
    const x11_setting *setting = xsettings_table_find(settings, rule->setting);
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
    bool is_dpi = strcmp(setting->name, xft_dpi_setting) == 0;
    snprintf(value, capacity, "%d", is_dpi ? setting->integer / XFT_DPI_UNIT : setting->integer);
    return true;
}

static void append_entry(text *out, const char *key, const char *value)
{
    append(out, key, strlen(key));
    append(out, ":\t", 2);
    append(out, value, strlen(value));
    append(out, "\n", 1);
}

static void append(text *out, const char *bytes, size_t length)
{
    if (out->length + length + 1 > out->capacity) {
        size_t capacity = (out->length + length + 1) * 2;
        char *grown = realloc(out->data, capacity);
        if (grown == NULL) {
            return;
        }
        out->data = grown;
        out->capacity = capacity;
    }
    memcpy(out->data + out->length, bytes, length);
    out->length += length;
    out->data[out->length] = '\0';
}
