#ifndef XFCE_KEYMAP_STORE_H
#define XFCE_KEYMAP_STORE_H

#include "wm/keys.h"

typedef struct keymap_store keymap_store;

keymap_store *keymap_store_create(void);
void keymap_store_destroy(keymap_store *store);
void keymap_store_add_action(keymap_store *store, wm_key key, wm_action action);
void keymap_store_add_command(keymap_store *store, wm_key key, const char *command);
void keymap_store_ensure_navigation(keymap_store *store);
wm_keymap keymap_store_view(const keymap_store *store);

#endif
