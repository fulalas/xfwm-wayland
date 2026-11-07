#ifndef XFCE_APP_SHORTCUTS_H
#define XFCE_APP_SHORTCUTS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "wm/keys.h"

typedef struct app_shortcuts app_shortcuts;

typedef struct app_shortcut_request {
    const char *id;
    const char *description;
    const char *trigger;
} app_shortcut_request;

typedef struct app_shortcut_requests {
    const app_shortcut_request *items;
    size_t count;
} app_shortcut_requests;

typedef struct app_shortcut_entry {
    uint32_t id;
    char *session;
    char *shortcut_id;
    char *description;
    char *trigger;
    wm_key key;
    bool usable;
    bool bound;
} app_shortcut_entry;

app_shortcuts *app_shortcuts_create(void);
void app_shortcuts_destroy(app_shortcuts *shortcuts);
void app_shortcuts_bind(app_shortcuts *shortcuts, const char *session,
                        app_shortcut_requests requests);
bool app_shortcuts_drop_session(app_shortcuts *shortcuts, const char *session);
size_t app_shortcuts_count(const app_shortcuts *shortcuts);
const app_shortcut_entry *app_shortcuts_at(const app_shortcuts *shortcuts, size_t index);
const app_shortcut_entry *app_shortcuts_find(const app_shortcuts *shortcuts, uint32_t id);
size_t app_shortcuts_keymap(const app_shortcuts *shortcuts, const wm_app_shortcut **keymap);

#endif
