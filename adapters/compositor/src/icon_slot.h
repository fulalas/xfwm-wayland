#ifndef COMPOSITOR_ICON_SLOT_H
#define COMPOSITOR_ICON_SLOT_H

#include <cairo.h>

#include "server.h"

typedef struct icon_slot icon_slot;

typedef struct icon_listener {
    void *self;
    void (*changed)(void *self);
} icon_listener;

icon_slot *icon_slot_create(compositor *server, const icon_listener *listener);
void icon_slot_destroy(icon_slot *slot);
void icon_slot_set(icon_slot *slot, cairo_surface_t *surface);
void icon_slot_lookup_name(icon_slot *slot, const char *app_id, const char *icon_name);
void icon_slot_lookup_app(icon_slot *slot, const char *app_id);
cairo_surface_t *icon_slot_surface(const icon_slot *slot);

#endif
