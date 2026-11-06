#ifndef XFCE_SHORTCUTS_PORTAL_H
#define XFCE_SHORTCUTS_PORTAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "wm/keys.h"

typedef struct shortcuts_portal shortcuts_portal;

typedef struct shortcuts_portal_listener {
    void *self;
    void (*changed)(void *self);
} shortcuts_portal_listener;

shortcuts_portal *shortcuts_portal_create(const shortcuts_portal_listener *listener);
void shortcuts_portal_destroy(shortcuts_portal *portal);
size_t shortcuts_portal_keymap(const shortcuts_portal *portal, const wm_app_shortcut **keymap);
void shortcuts_portal_activate(shortcuts_portal *portal, uint32_t id, bool pressed,
                               uint32_t time_ms);

#endif
