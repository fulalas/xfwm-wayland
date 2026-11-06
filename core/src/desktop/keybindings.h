#ifndef DESKTOP_KEYBINDINGS_H
#define DESKTOP_KEYBINDINGS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "wm/desktop.h"

typedef struct keybindings {
    wm_key_binding *bindings;
    size_t binding_count;
    wm_command_binding *commands;
    size_t command_count;
    wm_app_shortcut *app_shortcuts;
    size_t app_shortcut_count;
} keybindings;

void keybindings_free(keybindings *keys);
void keybindings_copy(keybindings *keys, const wm_keymap *keymap);
wm_action keybindings_action_for(const keybindings *keys, const wm_key_event *event);
const char *keybindings_command_for(const keybindings *keys, const wm_key_event *event);
const wm_app_shortcut *keybindings_app_shortcut_for(const keybindings *keys,
                                                     const wm_key_event *event);
bool keybindings_is_key_of(const keybindings *keys, wm_action action, uint32_t keysym);
uint32_t keybindings_modifiers_of(const keybindings *keys, wm_action action);
bool keysym_is_modifier(uint32_t keysym);
bool keysym_is_return(uint32_t keysym);

#endif
