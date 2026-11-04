#include <stdlib.h>

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
};

static void request_icon(icon_slot *slot, const char *app_id, const char *icon_name);
static void handle_icon_found(void *context, cairo_surface_t *icon);
static void replace_surface(icon_slot *slot, cairo_surface_t *surface);
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
    free(slot);
}

void icon_slot_set(icon_slot *slot, cairo_surface_t *surface)
{
    cancel_lookups(slot);
    slot->pinned = surface != NULL;
    replace_surface(slot, surface);
}

void icon_slot_lookup_name(icon_slot *slot, const char *app_id, const char *icon_name)
{
    slot->pinned = true;
    request_icon(slot, app_id, icon_name);
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
    if (icon != NULL) {
        replace_surface(slot, icon);
    }
}

static void replace_surface(icon_slot *slot, cairo_surface_t *surface)
{
    if (slot->surface == surface) {
        return;
    }
    if (slot->surface != NULL) {
        cairo_surface_destroy(slot->surface);
    }
    slot->surface = surface != NULL ? cairo_surface_reference(surface) : NULL;
    if (slot->listener.changed != NULL) {
        slot->listener.changed(slot->listener.self);
    }
}

static void cancel_lookups(icon_slot *slot)
{
    const compositor_services *services = slot->server->services;
    services->cancel_icon_lookups(services->self, slot);
}
