#include <stdlib.h>
#include <string.h>

#include "icon_slot.h"

enum {
    ICON_REQUEST_SIZE = 64,
};

static const char *const DEFAULT_ICON_NAME = "xfwm4-default";

struct icon_slot {
    compositor *server;
    icon_listener listener;
    cairo_surface_t *surface;
    bool pinned;
    bool app_pixels;
    char *app_name;
    unsigned serial;
};

static void request_icon(icon_slot *slot, const char *app_id, const char *icon_name);
static void handle_icon_found(void *context, cairo_surface_t *icon);
static void store_surface(icon_slot *slot, cairo_surface_t *surface);
static void store_name(icon_slot *slot, const char *name);
static void notify(icon_slot *slot);
static void cancel_lookups(icon_slot *slot);

icon_slot *icon_slot_create(compositor *server, const icon_listener *listener)
{
    icon_slot *slot = calloc(1, sizeof(*slot));
    if (slot == NULL) {
        return NULL;
    }
    slot->server = server;
    slot->listener = *listener;
    return slot;
}

void icon_slot_destroy(icon_slot *slot)
{
    cancel_lookups(slot);
    if (slot->surface != NULL) {
        cairo_surface_destroy(slot->surface);
    }
    free(slot->app_name);
    free(slot);
}

void icon_slot_set(icon_slot *slot, cairo_surface_t *surface)
{
    icon_slot_set_app(slot, &(app_icon){NULL, NULL, surface});
}

void icon_slot_set_app(icon_slot *slot, const app_icon *icon)
{
    cancel_lookups(slot);
    store_name(slot, icon->name);
    slot->app_pixels = icon->pixels != NULL;
    slot->pinned = icon->pixels != NULL || icon->name != NULL;
    slot->serial++;
    store_surface(slot, icon->pixels);
    const char *app_id = icon->app_id != NULL ? icon->app_id : "";
    if (icon->pixels == NULL && icon->name != NULL) {
        request_icon(slot, app_id, icon->name);
    } else if (icon->app_id != NULL) {
        icon_slot_lookup_app(slot, app_id);
    }
    notify(slot);
}

void icon_slot_lookup_app(icon_slot *slot, const char *app_id)
{
    if (!slot->pinned) {
        request_icon(slot, app_id != NULL ? app_id : "", DEFAULT_ICON_NAME);
    }
}

cairo_surface_t *icon_slot_surface(const icon_slot *slot)
{
    return slot != NULL ? slot->surface : NULL;
}

const char *icon_slot_app_name(const icon_slot *slot)
{
    return slot != NULL ? slot->app_name : NULL;
}

cairo_surface_t *icon_slot_app_pixels(const icon_slot *slot)
{
    return slot != NULL && slot->app_pixels ? slot->surface : NULL;
}

unsigned icon_slot_serial(const icon_slot *slot)
{
    return slot != NULL ? slot->serial : 0;
}

static void request_icon(icon_slot *slot, const char *app_id, const char *icon_name)
{
    const compositor_services *services = slot->server->services;
    cancel_lookups(slot);
    icon_request request = {
        .app_id = app_id,
        .icon_name = icon_name,
        .size = ICON_REQUEST_SIZE,
        .done = handle_icon_found,
        .context = slot,
    };
    services->lookup_icon(services->self, &request);
}

static void handle_icon_found(void *context, cairo_surface_t *icon)
{
    icon_slot *slot = context;
    if (icon != NULL && icon != slot->surface) {
        store_surface(slot, icon);
        notify(slot);
    }
}

static void store_surface(icon_slot *slot, cairo_surface_t *surface)
{
    if (slot->surface == surface) {
        return;
    }
    if (slot->surface != NULL) {
        cairo_surface_destroy(slot->surface);
    }
    slot->surface = surface != NULL ? cairo_surface_reference(surface) : NULL;
}

static void store_name(icon_slot *slot, const char *name)
{
    free(slot->app_name);
    slot->app_name = name != NULL ? strdup(name) : NULL;
}

static void notify(icon_slot *slot)
{
    if (slot->listener.changed != NULL) {
        slot->listener.changed(slot->listener.self);
    }
}

static void cancel_lookups(icon_slot *slot)
{
    const compositor_services *services = slot->server->services;
    services->cancel_icon_lookups(services->self, slot);
}
