#include "keybindings.h"

#include <stdlib.h>
#include <string.h>

enum {
    KEYSYM_MODIFIER_FIRST = 0xffe1,
    KEYSYM_MODIFIER_LAST = 0xffee,
    KEYSYM_ISO_MODIFIER_FIRST = 0xfe01,
    KEYSYM_ISO_MODIFIER_LAST = 0xfe13,
    KEYSYM_MODE_SWITCH = 0xff7e,
    KEYSYM_NUM_LOCK = 0xff7f,
};

static bool key_matches(const wm_key *key, const wm_key_event *event);
static uint32_t default_keysym(wm_action action);
static void copy_bindings(keybindings *keys, const wm_keymap *keymap);
static void copy_commands(keybindings *keys, const wm_keymap *keymap);
static char *copy_text(const char *text);

void keybindings_free(keybindings *keys)
{
    for (size_t i = 0; i < keys->command_count; i++) {
        free((char *) keys->commands[i].command);
    }
    free(keys->commands);
    free(keys->bindings);
    *keys = (keybindings) {0};
}

void keybindings_copy(keybindings *keys, const wm_keymap *keymap)
{
    keybindings_free(keys);
    if (keymap == NULL) {
        return;
    }
    copy_bindings(keys, keymap);
    copy_commands(keys, keymap);
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

static void copy_bindings(keybindings *keys, const wm_keymap *keymap)
{
    if (keymap->binding_count == 0) {
        return;
    }
    keys->bindings = malloc(keymap->binding_count * sizeof(keys->bindings[0]));
    if (keys->bindings == NULL) {
        return;
    }
    memcpy(keys->bindings, keymap->bindings, keymap->binding_count * sizeof(keys->bindings[0]));
    keys->binding_count = keymap->binding_count;
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
        keys->commands[i].command = copy_text(keymap->commands[i].command);
    }
    keys->command_count = keymap->command_count;
}

static char *copy_text(const char *text)
{
    size_t length = strlen(text ? text : "") + 1;
    char *copy = malloc(length);
    if (copy != NULL) {
        memcpy(copy, text ? text : "", length);
    }
    return copy;
}
