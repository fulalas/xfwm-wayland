#include "keybindings.h"

#include <stdlib.h>
#include <string.h>

enum {
    KEYSYM_MODIFIER_FIRST = 0xffe1,
    KEYSYM_CAPS_LOCK = 0xffe5,
    KEYSYM_SHIFT_LOCK = 0xffe6,
    KEYSYM_MODIFIER_LAST = 0xffee,
    KEYSYM_ISO_MODIFIER_FIRST = 0xfe01,
    KEYSYM_ISO_MODIFIER_LAST = 0xfe13,
    KEYSYM_MODE_SWITCH = 0xff7e,
    KEYSYM_NUM_LOCK = 0xff7f,
};

typedef struct copied_array {
    void *items;
    size_t count;
} copied_array;

static bool key_matches(const wm_key *key, const wm_key_event *event);
static uint32_t default_keysym(wm_action action);
static void copy_commands(keybindings *keys, const wm_keymap *keymap);
static copied_array copy_array(const void *items, size_t count, size_t size);
static char *copy_or_empty(const char *text);

void keybindings_free(keybindings *keys)
{
    for (size_t i = 0; i < keys->command_count; i++) {
        free((char *) keys->commands[i].command);
    }
    free(keys->commands);
    free(keys->bindings);
    free(keys->app_shortcuts);
    *keys = (keybindings) {0};
}

void keybindings_copy(keybindings *keys, const wm_keymap *keymap)
{
    keybindings_free(keys);
    if (keymap == NULL) {
        return;
    }
    copied_array bindings = copy_array(keymap->bindings, keymap->binding_count,
                                       sizeof(keys->bindings[0]));
    keys->bindings = bindings.items;
    keys->binding_count = bindings.count;
    copy_commands(keys, keymap);
    copied_array shortcuts = copy_array(keymap->app_shortcuts, keymap->app_shortcut_count,
                                        sizeof(keys->app_shortcuts[0]));
    keys->app_shortcuts = shortcuts.items;
    keys->app_shortcut_count = shortcuts.count;
}

wm_action keybindings_action_for(const keybindings *keys, const wm_key_event *event)
{
    for (size_t i = 0; i < keys->binding_count; i++) {
        if (key_matches(&keys->bindings[i].key, event)) {
            return keys->bindings[i].action;
        }
    }
    return WM_ACTION_NONE;
}

const char *keybindings_command_for(const keybindings *keys, const wm_key_event *event)
{
    for (size_t i = 0; i < keys->command_count; i++) {
        if (key_matches(&keys->commands[i].key, event)) {
            return keys->commands[i].command;
        }
    }
    return NULL;
}

const wm_app_shortcut *keybindings_app_shortcut_for(const keybindings *keys,
                                                     const wm_key_event *event)
{
    for (size_t i = 0; i < keys->app_shortcut_count; i++) {
        if (key_matches(&keys->app_shortcuts[i].key, event)) {
            return &keys->app_shortcuts[i];
        }
    }
    return NULL;
}

bool keybindings_is_key_of(const keybindings *keys, wm_action action, uint32_t keysym)
{
    bool bound = false;
    for (size_t i = 0; i < keys->binding_count; i++) {
        if (keys->bindings[i].action != action) {
            continue;
        }
        bound = true;
        if (keys->bindings[i].key.keysym == keysym) {
            return true;
        }
    }
    return !bound && default_keysym(action) == keysym;
}

move_direction keybindings_direction_of(const keybindings *keys, uint32_t keysym)
{
    static const struct {
        wm_action action;
        move_direction direction;
    } arrows[] = {
        {WM_ACTION_LEFT, DIRECTION_LEFT},
        {WM_ACTION_RIGHT, DIRECTION_RIGHT},
        {WM_ACTION_UP, DIRECTION_UP},
        {WM_ACTION_DOWN, DIRECTION_DOWN},
    };
    for (size_t i = 0; i < sizeof(arrows) / sizeof(arrows[0]); i++) {
        if (keybindings_is_key_of(keys, arrows[i].action, keysym)) {
            return arrows[i].direction;
        }
    }
    return DIRECTION_NONE;
}

uint32_t keybindings_modifiers_of(const keybindings *keys, wm_action action)
{
    uint32_t modifiers = 0;
    for (size_t i = 0; i < keys->binding_count; i++) {
        if (keys->bindings[i].action == action) {
            modifiers |= keys->bindings[i].key.modifiers;
        }
    }
    return modifiers;
}

bool keysym_is_modifier(uint32_t keysym)
{
    return (keysym >= KEYSYM_MODIFIER_FIRST && keysym <= KEYSYM_MODIFIER_LAST)
           || (keysym >= KEYSYM_ISO_MODIFIER_FIRST && keysym <= KEYSYM_ISO_MODIFIER_LAST)
           || keysym == KEYSYM_MODE_SWITCH || keysym == KEYSYM_NUM_LOCK;
}

bool keysym_is_held_modifier(uint32_t keysym)
{
    return keysym >= KEYSYM_MODIFIER_FIRST && keysym <= KEYSYM_MODIFIER_LAST
           && keysym != KEYSYM_CAPS_LOCK && keysym != KEYSYM_SHIFT_LOCK;
}

bool keysym_is_return(uint32_t keysym)
{
    return keysym == WM_KEYSYM_RETURN || keysym == WM_KEYSYM_KP_ENTER;
}

static bool key_matches(const wm_key *key, const wm_key_event *event)
{
    return key->keysym != 0 && key->keysym == event->keysym && key->modifiers == event->modifiers;
}

static uint32_t default_keysym(wm_action action)
{
    switch (action) {
    case WM_ACTION_CANCEL:
        return WM_KEYSYM_ESCAPE;
    case WM_ACTION_LEFT:
        return WM_KEYSYM_LEFT;
    case WM_ACTION_RIGHT:
        return WM_KEYSYM_RIGHT;
    case WM_ACTION_UP:
        return WM_KEYSYM_UP;
    case WM_ACTION_DOWN:
        return WM_KEYSYM_DOWN;
    default:
        return 0;
    }
}

static copied_array copy_array(const void *items, size_t count, size_t size)
{
    copied_array copy = {count > 0 ? malloc(count * size) : NULL, 0};
    if (copy.items != NULL) {
        memcpy(copy.items, items, count * size);
        copy.count = count;
    }
    return copy;
}

static void copy_commands(keybindings *keys, const wm_keymap *keymap)
{
    if (keymap->command_count == 0) {
        return;
    }
    keys->commands = calloc(keymap->command_count, sizeof(keys->commands[0]));
    if (keys->commands == NULL) {
        return;
    }
    for (size_t i = 0; i < keymap->command_count; i++) {
        keys->commands[i].key = keymap->commands[i].key;
        keys->commands[i].command = copy_or_empty(keymap->commands[i].command);
    }
    keys->command_count = keymap->command_count;
}

static char *copy_or_empty(const char *text)
{
    size_t length = strlen(text ? text : "") + 1;
    char *copy = malloc(length);
    if (copy != NULL) {
        memcpy(copy, text ? text : "", length);
    }
    return copy;
}
