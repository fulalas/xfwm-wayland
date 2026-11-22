#include "system_keymap.h"

#include <glib.h>
#include <stdbool.h>
#include <string.h>

typedef struct field_names {
    const char *vconsole;
    const char *xorg;
} field_names;

typedef struct xorg_section {
    bool input_class;
    bool other_match;
    char *fields[SYSTEM_KEYMAP_FIELDS];
} xorg_section;

static const field_names names[SYSTEM_KEYMAP_FIELDS] = {
    [SYSTEM_KEYMAP_LAYOUT] = { "XKBLAYOUT", "xkblayout" },
    [SYSTEM_KEYMAP_VARIANT] = { "XKBVARIANT", "xkbvariant" },
    [SYSTEM_KEYMAP_MODEL] = { "XKBMODEL", "xkbmodel" },
    [SYSTEM_KEYMAP_OPTIONS] = { "XKBOPTIONS", "xkboptions" },
};

static const char *const xkb_environment[] = {
    "XKB_DEFAULT_RULES", "XKB_DEFAULT_MODEL", "XKB_DEFAULT_LAYOUT", "XKB_DEFAULT_VARIANT",
    "XKB_DEFAULT_OPTIONS",
};
static const char *const all_keyboards_matches[] = { "MatchDriver", "MatchOS" };
static const char any_event_device[] = "/dev/input/event*";

static void read_vconsole_line(system_keymap *keymap, char *line);
static void read_xorg_line(system_keymap *keymap, xorg_section *section, const char *line);
static void read_xorg_entry(xorg_section *section, char **words);
static void end_section(system_keymap *keymap, xorg_section *section);
static bool matches_all_keyboards(char **words);
static bool has_layout(const system_keymap *keymap);
static bool has_xkb_environment(char **environment);
static int vconsole_field(const char *key);
static int xorg_field(const char *option);
static bool is_true(const char *text);
static void set_field(char **field, const char *value);

void system_keymap_read_vconsole(system_keymap *keymap, const char *text)
{
    char **lines = g_strsplit(text != NULL ? text : "", "\n", -1);
    for (char **line = lines; *line != NULL; line++) {
        read_vconsole_line(keymap, *line);
    }
    g_strfreev(lines);
}

void system_keymap_read_xorg(system_keymap *keymap, const char *text)
{
    xorg_section section = { 0 };
    char **lines = g_strsplit(text != NULL ? text : "", "\n", -1);
    for (char **line = lines; *line != NULL; line++) {
        read_xorg_line(keymap, &section, *line);
    }
    g_strfreev(lines);
    for (int i = 0; i < SYSTEM_KEYMAP_FIELDS; i++) {
        g_free(section.fields[i]);
    }
}

system_keymap system_keymap_choose(system_keymap *vconsole, system_keymap *xorg,
                                    char **environment)
{
    system_keymap chosen = { 0 };
    if (!has_xkb_environment(environment)) {
        system_keymap *source = has_layout(vconsole) ? vconsole : has_layout(xorg) ? xorg : NULL;
        if (source != NULL) {
            chosen = *source;
            *source = (system_keymap) { 0 };
        }
    }
    system_keymap_clear(vconsole);
    system_keymap_clear(xorg);
    return chosen;
}

void system_keymap_clear(system_keymap *keymap)
{
    for (int i = 0; i < SYSTEM_KEYMAP_FIELDS; i++) {
        g_clear_pointer(&keymap->fields[i], g_free);
    }
}

static void read_vconsole_line(system_keymap *keymap, char *line)
{
    char *equals = strchr(line, '=');
    if (equals == NULL) {
        return;
    }
    *equals = '\0';
    int field = vconsole_field(g_strstrip(line));
    char *value = field >= 0 ? g_shell_unquote(g_strstrip(equals + 1), NULL) : NULL;
    if (value != NULL) {
        set_field(&keymap->fields[field], value);
    }
    g_free(value);
}

static void read_xorg_line(system_keymap *keymap, xorg_section *section, const char *line)
{
    char **words = NULL;
    if (!g_shell_parse_argv(line, NULL, &words, NULL)) {
        return;
    }
    if (g_ascii_strcasecmp(words[0], "Section") == 0) {
        end_section(keymap, section);
        section->input_class = words[1] != NULL && g_ascii_strcasecmp(words[1], "InputClass") == 0;
    } else if (g_ascii_strcasecmp(words[0], "EndSection") == 0) {
        end_section(keymap, section);
    } else if (section->input_class) {
        read_xorg_entry(section, words);
    }
    g_strfreev(words);
}

static void read_xorg_entry(xorg_section *section, char **words)
{
    if (g_ascii_strncasecmp(words[0], "Match", strlen("Match")) == 0) {
        section->other_match |= !matches_all_keyboards(words);
    } else if (g_ascii_strcasecmp(words[0], "Option") == 0 && words[1] != NULL) {
        int field = xorg_field(words[1]);
        if (field >= 0) {
            set_field(&section->fields[field], words[2] != NULL ? words[2] : "");
        }
    }
}

static void end_section(system_keymap *keymap, xorg_section *section)
{
    for (int i = 0; i < SYSTEM_KEYMAP_FIELDS; i++) {
        if (section->input_class && !section->other_match && section->fields[i] != NULL) {
            set_field(&keymap->fields[i], section->fields[i]);
        }
        g_free(section->fields[i]);
    }
    *section = (xorg_section) { 0 };
}

static bool matches_all_keyboards(char **words)
{
    const char *value = words[1];
    if (g_ascii_strcasecmp(words[0], "MatchIsKeyboard") == 0) {
        return is_true(value);
    }
    if (g_ascii_strncasecmp(words[0], "MatchIs", strlen("MatchIs")) == 0) {
        return !is_true(value);
    }
    if (g_ascii_strcasecmp(words[0], "MatchDevicePath") == 0) {
        return g_strcmp0(value, any_event_device) == 0;
    }
    for (size_t i = 0; i < G_N_ELEMENTS(all_keyboards_matches); i++) {
        if (g_ascii_strcasecmp(words[0], all_keyboards_matches[i]) == 0) {
            return true;
        }
    }
    return false;
}

static bool has_layout(const system_keymap *keymap)
{
    const char *layout = keymap->fields[SYSTEM_KEYMAP_LAYOUT];
    return layout != NULL && layout[0] != '\0';
}

static bool has_xkb_environment(char **environment)
{
    for (size_t i = 0; i < G_N_ELEMENTS(xkb_environment); i++) {
        if (g_environ_getenv(environment, xkb_environment[i]) != NULL) {
            return true;
        }
    }
    return false;
}

static int vconsole_field(const char *key)
{
    for (int i = 0; i < SYSTEM_KEYMAP_FIELDS; i++) {
        if (strcmp(names[i].vconsole, key) == 0) {
            return i;
        }
    }
    return -1;
}

static int xorg_field(const char *option)
{
    GString *name = g_string_new(NULL);
    for (const char *c = option; *c != '\0'; c++) {
        if (*c != '_' && *c != ' ' && *c != '\t') {
            g_string_append_c(name, g_ascii_tolower(*c));
        }
    }
    int found = -1;
    for (int i = 0; i < SYSTEM_KEYMAP_FIELDS && found < 0; i++) {
        if (strcmp(names[i].xorg, name->str) == 0) {
            found = i;
        }
    }
    g_string_free(name, TRUE);
    return found;
}

static bool is_true(const char *text)
{
    static const char *const truths[] = { "on", "true", "yes", "1" };
    for (size_t i = 0; text != NULL && i < G_N_ELEMENTS(truths); i++) {
        if (g_ascii_strcasecmp(text, truths[i]) == 0) {
            return true;
        }
    }
    return false;
}

static void set_field(char **field, const char *value)
{
    g_free(*field);
    *field = g_strdup(value);
}
