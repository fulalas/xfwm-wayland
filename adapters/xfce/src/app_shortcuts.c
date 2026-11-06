#include "app_shortcuts.h"

#include <glib.h>
#include <string.h>

#include "accelerator.h"

struct app_shortcuts {
    GPtrArray *entries;
    GArray *keymap;
    uint32_t next_id;
};

static void entry_free(gpointer data);
static bool remove_session(app_shortcuts *shortcuts, const char *session);
static bool key_taken(const app_shortcuts *shortcuts, wm_key key);
static void rebuild_keymap(app_shortcuts *shortcuts);

app_shortcuts *app_shortcuts_create(void)
{
    app_shortcuts *shortcuts = g_new0(app_shortcuts, 1);

    shortcuts->entries = g_ptr_array_new_with_free_func(entry_free);
    shortcuts->keymap = g_array_new(FALSE, FALSE, sizeof(wm_app_shortcut));
    shortcuts->next_id = 1;
    return shortcuts;
}

void app_shortcuts_destroy(app_shortcuts *shortcuts)
{
    if (shortcuts == NULL) {
        return;
    }
    g_ptr_array_unref(shortcuts->entries);
    g_array_unref(shortcuts->keymap);
    g_free(shortcuts);
}

void app_shortcuts_bind(app_shortcuts *shortcuts, const char *session,
                        const app_shortcut_request *requests, size_t count)
{
    remove_session(shortcuts, session);
    for (size_t i = 0; i < count; i++) {
        app_shortcut_entry *entry = g_new0(app_shortcut_entry, 1);
        entry->id = shortcuts->next_id++;
        entry->session = g_strdup(session);
        entry->shortcut_id = g_strdup(requests[i].id);
        const char *description = requests[i].description;
        entry->description = g_strdup(description != NULL ? description : "");
        entry->usable = accelerator_parse_trigger(requests[i].trigger, &entry->key)
                        && wm_key_is_global(&entry->key);
        entry->trigger = g_strdup(entry->usable ? requests[i].trigger : "");
        g_ptr_array_add(shortcuts->entries, entry);
    }
    rebuild_keymap(shortcuts);
}

bool app_shortcuts_drop_session(app_shortcuts *shortcuts, const char *session)
{
    bool dropped = remove_session(shortcuts, session);

    rebuild_keymap(shortcuts);
    return dropped;
}

size_t app_shortcuts_count(const app_shortcuts *shortcuts)
{
    return shortcuts->entries->len;
}

const app_shortcut_entry *app_shortcuts_at(const app_shortcuts *shortcuts, size_t index)
{
    return index < shortcuts->entries->len ? g_ptr_array_index(shortcuts->entries, index) : NULL;
}

const app_shortcut_entry *app_shortcuts_find(const app_shortcuts *shortcuts, uint32_t id)
{
    for (guint i = 0; i < shortcuts->entries->len; i++) {
        const app_shortcut_entry *entry = g_ptr_array_index(shortcuts->entries, i);
        if (entry->id == id) {
            return entry;
        }
    }
    return NULL;
}

size_t app_shortcuts_keymap(const app_shortcuts *shortcuts, const wm_app_shortcut **keymap)
{
    *keymap = (const wm_app_shortcut *) (const void *) shortcuts->keymap->data;
    return shortcuts->keymap->len;
}

static void entry_free(gpointer data)
{
    app_shortcut_entry *entry = data;

    g_free(entry->session);
    g_free(entry->shortcut_id);
    g_free(entry->description);
    g_free(entry->trigger);
    g_free(entry);
}

static bool remove_session(app_shortcuts *shortcuts, const char *session)
{
    bool removed = false;

    for (guint i = shortcuts->entries->len; i > 0; i--) {
        const app_shortcut_entry *entry = g_ptr_array_index(shortcuts->entries, i - 1);
        if (strcmp(entry->session, session) == 0) {
            g_ptr_array_remove_index(shortcuts->entries, i - 1);
            removed = true;
        }
    }
    return removed;
}

static bool key_taken(const app_shortcuts *shortcuts, wm_key key)
{
    for (guint i = 0; i < shortcuts->entries->len; i++) {
        const app_shortcut_entry *entry = g_ptr_array_index(shortcuts->entries, i);
        if (entry->bound && entry->key.keysym == key.keysym
            && entry->key.modifiers == key.modifiers) {
            return true;
        }
    }
    return false;
}

static void rebuild_keymap(app_shortcuts *shortcuts)
{
    g_array_set_size(shortcuts->keymap, 0);
    for (guint i = 0; i < shortcuts->entries->len; i++) {
        ((app_shortcut_entry *) g_ptr_array_index(shortcuts->entries, i))->bound = false;
    }
    for (guint i = 0; i < shortcuts->entries->len; i++) {
        app_shortcut_entry *entry = g_ptr_array_index(shortcuts->entries, i);
        entry->bound = entry->usable && !key_taken(shortcuts, entry->key);
        if (entry->bound) {
            wm_app_shortcut binding = { entry->key, entry->id };
            g_array_append_val(shortcuts->keymap, binding);
        }
    }
}
