#include "shortcuts.h"

#include "accelerator.h"
#include "shortcut_defaults.h"
#include "shortcut_property.h"

typedef void (*shortcut_adder)(keymap_store *store, wm_key key, const char *value);

typedef struct shortcut_group {
    const char *root;
    shortcut_adder add;
} shortcut_group;

typedef struct group_target {
    keymap_store *store;
    const char *base;
    shortcut_adder add;
} group_target;

static void add_action(keymap_store *store, wm_key key, const char *value);
static void add_command(keymap_store *store, wm_key key, const char *value);
static bool has_group(XfconfChannel *channel, const char *root);
static void load_group(keymap_store *store, XfconfChannel *channel, const shortcut_group *group);
static void add_entries(const group_target *target, GHashTable *properties);
static void add_entry(const group_target *target, const char *accelerator, const char *value);
static void add_builtin_actions(keymap_store *store);

static const shortcut_group action_group = { "/xfwm4", add_action };
static const shortcut_group command_group = { "/commands", add_command };

keymap_store *shortcuts_load(XfconfChannel *channel)
{
    keymap_store *store = keymap_store_create();

    if (channel && has_group(channel, action_group.root)) {
        load_group(store, channel, &action_group);
    } else {
        add_builtin_actions(store);
    }
    if (channel) {
        load_group(store, channel, &command_group);
    }
    keymap_store_ensure_navigation(store);
    return store;
}

static void add_action(keymap_store *store, wm_key key, const char *value)
{
    keymap_store_add_action(store, key, wm_action_from_name(value));
}

static void add_command(keymap_store *store, wm_key key, const char *value)
{
    keymap_store_add_command(store, key, value);
}

static bool has_group(XfconfChannel *channel, const char *root)
{
    GHashTable *properties = xfconf_channel_get_properties(channel, root);
    bool found = properties && g_hash_table_size(properties) > 0;

    if (properties) {
        g_hash_table_destroy(properties);
    }
    return found;
}

static void load_group(keymap_store *store, XfconfChannel *channel, const shortcut_group *group)
{
    char *override = g_strconcat(group->root, "/custom/override", NULL);
    bool is_custom = xfconf_channel_get_bool(channel, override, FALSE);
    char *base = g_strconcat(group->root, is_custom ? "/custom" : "/default", NULL);
    GHashTable *properties = xfconf_channel_get_properties(channel, base);

    if (properties) {
        group_target target = { .store = store, .base = base, .add = group->add };
        add_entries(&target, properties);
        g_hash_table_destroy(properties);
    }
    g_free(base);
    g_free(override);
}

static void add_entries(const group_target *target, GHashTable *properties)
{
    GHashTableIter iter;
    void *name;
    void *value;

    g_hash_table_iter_init(&iter, properties);
    while (g_hash_table_iter_next(&iter, &name, &value)) {
        const char *accelerator = shortcut_property_accelerator(name, target->base);
        if (accelerator && G_VALUE_HOLDS_STRING(value)) {
            add_entry(target, accelerator, g_value_get_string(value));
        }
    }
}

static void add_entry(const group_target *target, const char *accelerator, const char *value)
{
    wm_key key;

    if (value && *value && accelerator_parse(accelerator, &key)) {
        target->add(target->store, key, value);
    }
}

static void add_builtin_actions(keymap_store *store)
{
    size_t count;
    const shortcut_default *table = shortcut_defaults_table(&count);
    group_target target = { .store = store, .base = NULL, .add = add_action };

    for (size_t i = 0; i < count; i++) {
        add_entry(&target, table[i].accelerator, table[i].action);
    }
}
