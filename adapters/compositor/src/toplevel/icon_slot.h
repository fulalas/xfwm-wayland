#ifndef COMPOSITOR_ICON_SLOT_H
#define COMPOSITOR_ICON_SLOT_H

#include <cairo.h>

#include "server/server.h"

typedef struct icon_slot icon_slot;

typedef struct icon_listener {
    void *self;
    void (*changed)(void *self);
} icon_listener;

typedef struct app_icon {
    const char *app_id;
    const char *name;
    cairo_surface_t *pixels;
} app_icon;

icon_slot *icon_slot_create(compositor *server, const icon_listener *listener);
void icon_slot_destroy(icon_slot *slot);
void icon_slot_set(icon_slot *slot, cairo_surface_t *surface);
void icon_slot_set_app(icon_slot *slot, const app_icon *icon);
void icon_slot_lookup_app(icon_slot *slot, const char *app_id);
void icon_slot_reload(icon_slot *slot);
cairo_surface_t *icon_slot_surface(const icon_slot *slot);
cairo_surface_t *icon_slot_surface_at(icon_slot *slot, int pixels);
const char *icon_slot_app_name(const icon_slot *slot);
cairo_surface_t *icon_slot_app_pixels(const icon_slot *slot);
unsigned icon_slot_serial(const icon_slot *slot);

#endif
