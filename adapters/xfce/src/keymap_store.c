#include "keymap_store.h"

#include <glib.h>
#include <stdbool.h>

struct keymap_store {
    GArray *bindings;
    GArray *commands;
};

typedef struct navigation_key {
    uint32_t keysym;
    wm_action action;
} navigation_key;

static const navigation_key navigation_keys[] = {
    { WM_KEYSYM_ESCAPE, WM_ACTION_CANCEL },
    { WM_KEYSYM_UP, WM_ACTION_UP },
    { WM_KEYSYM_DOWN, WM_ACTION_DOWN },
    { WM_KEYSYM_LEFT, WM_ACTION_LEFT },
    { WM_KEYSYM_RIGHT, WM_ACTION_RIGHT },
};

static bool has_binding(const keymap_store *store, wm_key key, wm_action action);
static void free_command(void *data);

keymap_store *keymap_store_create(void)
{
    keymap_store *store = g_new0(keymap_store, 1);

    store->bindings = g_array_new(FALSE, FALSE, sizeof(wm_key_binding));
    store->commands = g_array_new(FALSE, FALSE, sizeof(wm_command_binding));
    g_array_set_clear_func(store->commands, free_command);
    return store;
}

void keymap_store_destroy(keymap_store *store)
{
    g_array_free(store->bindings, TRUE);
    g_array_free(store->commands, TRUE);
    g_free(store);
}

void keymap_store_add_action(keymap_store *store, wm_key key, wm_action action)
{
    wm_key_binding binding = { .key = key, .action = action };

    if (action == WM_ACTION_NONE || has_binding(store, key, action)) {
        return;
    }
    g_array_append_val(store->bindings, binding);
}

void keymap_store_add_command(keymap_store *store, wm_key key, const char *command)
{
    wm_command_binding binding = { .key = key, .command = g_strdup(command) };

    g_array_append_val(store->commands, binding);
}

void keymap_store_ensure_navigation(keymap_store *store)
{
    for (size_t i = 0; i < G_N_ELEMENTS(navigation_keys); i++) {
        wm_key key = { .keysym = navigation_keys[i].keysym, .modifiers = 0 };
        keymap_store_add_action(store, key, navigation_keys[i].action);
    }
}

wm_keymap keymap_store_view(const keymap_store *store)
{
    return (wm_keymap) {
        .bindings = (const wm_key_binding *) (void *) store->bindings->data,
        .binding_count = store->bindings->len,
        .commands = (const wm_command_binding *) (void *) store->commands->data,
        .command_count = store->commands->len,
    };
}

static bool has_binding(const keymap_store *store, wm_key key, wm_action action)
{
    for (guint i = 0; i < store->bindings->len; i++) {
        const wm_key_binding *binding = &g_array_index(store->bindings, wm_key_binding, i);
        if (binding->action == action && binding->key.keysym == key.keysym &&
            binding->key.modifiers == key.modifiers) {
            return true;
        }
    }
    return false;
}

static void free_command(void *data)
{
    wm_command_binding *binding = data;

    g_free((char *) binding->command);
}
