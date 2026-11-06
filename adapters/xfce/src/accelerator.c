#include "accelerator.h"

#include <glib.h>
#include <string.h>
#include <xkbcommon/xkbcommon.h>

enum {
    MODIFIER_NAME_MAX = 32,
};

typedef struct modifier_name {
    const char *name;
    uint32_t modifier;
} modifier_name;

static const modifier_name modifier_names[] = {
    { "Primary", WM_MOD_CTRL },
    { "Control", WM_MOD_CTRL },
    { "Ctrl", WM_MOD_CTRL },
    { "Ctl", WM_MOD_CTRL },
    { "Shift", WM_MOD_SHIFT },
    { "Shft", WM_MOD_SHIFT },
    { "Alt", WM_MOD_ALT },
    { "Mod1", WM_MOD_ALT },
    { "Super", WM_MOD_SUPER },
    { "Mod4", WM_MOD_SUPER },
    { "Hyper", WM_MOD_SUPER },
    { "Meta", WM_MOD_SUPER },
    { "Logo", WM_MOD_SUPER },
    { "Mod3", WM_MOD_MOD3 },
    { "Mod5", WM_MOD_MOD5 },
};

static const char *skip_modifiers(const char *text, uint32_t *modifiers);
static const char *skip_one_modifier(const char *text, uint32_t *modifiers);
static uint32_t keysym_from_name(const char *name);
static bool make_key(const char *name, uint32_t modifiers, wm_key *key);
static bool is_disabled(const char *text);

bool accelerator_parse(const char *text, wm_key *key)
{
    uint32_t modifiers = 0;
    const char *name;

    if (!text || is_disabled(text)) {
        return false;
    }
    name = skip_modifiers(text, &modifiers);
    return name != NULL && make_key(name, modifiers, key);
}

bool accelerator_parse_trigger(const char *text, wm_key *key)
{
    if (text == NULL || *text == '\0') {
        return false;
    }
    char **parts = g_strsplit(text, "+", -1);
    guint last = g_strv_length(parts) - 1;
    uint32_t modifiers = 0;
    bool parsed = true;

    for (guint i = 0; parsed && i < last; i++) {
        const char *name = g_strstrip(parts[i]);
        uint32_t modifier = accelerator_modifier_from_name(name);
        parsed = modifier != 0 || g_ascii_strcasecmp(name, "NUM") == 0;
        modifiers |= modifier;
    }
    parsed = parsed && make_key(g_strstrip(parts[last]), modifiers, key);
    g_strfreev(parts);
    return parsed;
}

uint32_t accelerator_modifier_from_name(const char *name)
{
    for (size_t i = 0; i < G_N_ELEMENTS(modifier_names); i++) {
        if (g_ascii_strcasecmp(name, modifier_names[i].name) == 0) {
            return modifier_names[i].modifier;
        }
    }
    return 0;
}

static const char *skip_modifiers(const char *text, uint32_t *modifiers)
{
    while (text && *text == '<') {
        text = skip_one_modifier(text, modifiers);
    }
    return text;
}

static const char *skip_one_modifier(const char *text, uint32_t *modifiers)
{
    const char *end = strchr(text, '>');
    char name[MODIFIER_NAME_MAX];
    size_t length;

    if (!end) {
        return NULL;
    }
    length = (size_t) (end - text - 1);
    if (length < sizeof(name)) {
        memcpy(name, text + 1, length);
        name[length] = '\0';
        *modifiers |= accelerator_modifier_from_name(name);
    }
    return end + 1;
}

static uint32_t keysym_from_name(const char *name)
{
    xkb_keysym_t keysym;

    if (*name == '\0') {
        return XKB_KEY_NoSymbol;
    }
    keysym = xkb_keysym_from_name(name, XKB_KEYSYM_NO_FLAGS);
    if (keysym == XKB_KEY_NoSymbol) {
        keysym = xkb_keysym_from_name(name, XKB_KEYSYM_CASE_INSENSITIVE);
    }
    return keysym;
}

static bool make_key(const char *name, uint32_t modifiers, wm_key *key)
{
    uint32_t keysym = keysym_from_name(name);

    if (keysym == XKB_KEY_NoSymbol) {
        return false;
    }
    *key = (wm_key) { .keysym = xkb_keysym_to_lower(keysym), .modifiers = modifiers };
    return true;
}

static bool is_disabled(const char *text)
{
    return g_ascii_strcasecmp(text, "none") == 0 || g_ascii_strcasecmp(text, "disabled") == 0;
}
