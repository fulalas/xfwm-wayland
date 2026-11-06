#include <stdlib.h>
#include <string.h>

#include "icon_slot.h"

enum {
    ICON_REQUEST_SIZE = 64,
    SIZED_ICON_MAX = 4,
};

static const char *const DEFAULT_ICON_NAME = "xfwm4-default";

typedef struct sized_icon {
    icon_slot *slot;
    int pixels;
    cairo_surface_t *surface;
} sized_icon;

struct icon_slot {
    compositor *server;
    icon_listener listener;
    cairo_surface_t *surface;
    bool pinned;
    bool app_pixels;
    char *app_name;
    unsigned serial;
    char *lookup_app_id;
    char *lookup_name;
    sized_icon sized[SIZED_ICON_MAX];
    size_t next_sized;
};

typedef void (*icon_done)(void *context, cairo_surface_t *icon);

static void request_icon(icon_slot *slot, const char *app_id, const char *icon_name);
static void request_sized(sized_icon *entry);
static sized_icon *sized_entry(icon_slot *slot, int pixels);
static void lookup(icon_slot *slot, void *context, int size, icon_done done);
static void handle_icon_found(void *context, cairo_surface_t *icon);
static void handle_sized_found(void *context, cairo_surface_t *icon);
static void store_found(icon_slot *slot, cairo_surface_t **target, cairo_surface_t *icon);
static void forget_lookup(icon_slot *slot);
static void replace_surface(cairo_surface_t **target, cairo_surface_t *surface);
static void store_name(icon_slot *slot, const char *name);
static void notify(icon_slot *slot);
static void cancel_lookups(icon_slot *slot);
static void cancel_lookup(icon_slot *slot, void *context);

icon_slot *icon_slot_create(compositor *server, const icon_listener *listener)
{
    icon_slot *slot = calloc(1, sizeof(*slot));
    if (slot == NULL) {
        return NULL;
    }
    slot->server = server;
    slot->listener = *listener;
    for (size_t i = 0; i < SIZED_ICON_MAX; i++) {
        slot->sized[i].slot = slot;
    }
    return slot;
}

void icon_slot_destroy(icon_slot *slot)
{
    cancel_lookups(slot);
    replace_surface(&slot->surface, NULL);
    forget_lookup(slot);
    free(slot->app_name);
    free(slot);
}

void icon_slot_set(icon_slot *slot, cairo_surface_t *surface)
{
    icon_slot_set_app(slot, &(app_icon){NULL, NULL, surface});
}

void icon_slot_set_app(icon_slot *slot, const app_icon *icon)
{
    store_name(slot, icon->name);
    slot->app_pixels = icon->pixels != NULL;
    slot->pinned = icon->pixels != NULL || icon->name != NULL;
    slot->serial++;
    replace_surface(&slot->surface, icon->pixels);
    const char *app_id = icon->app_id != NULL ? icon->app_id : "";
    if (icon->pixels == NULL && icon->name != NULL) {
        request_icon(slot, app_id, icon->name);
    } else {
        cancel_lookups(slot);
        forget_lookup(slot);
        if (icon->app_id != NULL) {
            icon_slot_lookup_app(slot, app_id);
        }
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

cairo_surface_t *icon_slot_surface_at(icon_slot *slot, int pixels)
{
    if (slot == NULL) {
        return NULL;
    }
    if (pixels <= 0 || pixels == ICON_REQUEST_SIZE || slot->lookup_name == NULL) {
        return slot->surface;
    }
    sized_icon *entry = sized_entry(slot, pixels);
    return entry->surface != NULL ? entry->surface : slot->surface;
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
    cancel_lookups(slot);
    char *app_id_copy = strdup(app_id);
    char *name_copy = strdup(icon_name);
    forget_lookup(slot);
    slot->lookup_app_id = app_id_copy;
    slot->lookup_name = name_copy;
    lookup(slot, slot, ICON_REQUEST_SIZE, handle_icon_found);
}

static void request_sized(sized_icon *entry)
{
    if (entry->pixels > 0 && entry->slot->lookup_name != NULL) {
        lookup(entry->slot, entry, entry->pixels, handle_sized_found);
    }
}

static sized_icon *sized_entry(icon_slot *slot, int pixels)
{
    for (size_t i = 0; i < SIZED_ICON_MAX; i++) {
        if (slot->sized[i].pixels == pixels) {
            return &slot->sized[i];
        }
    }
    sized_icon *entry = &slot->sized[slot->next_sized];
    slot->next_sized = (slot->next_sized + 1) % SIZED_ICON_MAX;
    cancel_lookup(slot, entry);
    replace_surface(&entry->surface, NULL);
    entry->pixels = pixels;
    request_sized(entry);
    return entry;
}

static void lookup(icon_slot *slot, void *context, int size, icon_done done)
{
    const compositor_services *services = slot->server->services;
    icon_request request = {
        .app_id = slot->lookup_app_id,
        .icon_name = slot->lookup_name,
        .size = size,
        .done = done,
        .context = context,
    };
    services->lookup_icon(services->self, &request);
}

static void handle_icon_found(void *context, cairo_surface_t *icon)
{
    icon_slot *slot = context;
    store_found(slot, &slot->surface, icon);
}

static void handle_sized_found(void *context, cairo_surface_t *icon)
{
    sized_icon *entry = context;
    store_found(entry->slot, &entry->surface, icon);
}

static void store_found(icon_slot *slot, cairo_surface_t **target, cairo_surface_t *icon)
{
    if (icon != NULL && icon != *target) {
        replace_surface(target, icon);
        notify(slot);
    }
}

static void forget_lookup(icon_slot *slot)
{
    free(slot->lookup_app_id);
    free(slot->lookup_name);
    slot->lookup_app_id = NULL;
    slot->lookup_name = NULL;
    for (size_t i = 0; i < SIZED_ICON_MAX; i++) {
        replace_surface(&slot->sized[i].surface, NULL);
        slot->sized[i].pixels = 0;
    }
}

static void replace_surface(cairo_surface_t **target, cairo_surface_t *surface)
{
    if (*target == surface) {
        return;
    }
    if (*target != NULL) {
        cairo_surface_destroy(*target);
    }
    *target = surface != NULL ? cairo_surface_reference(surface) : NULL;
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
    cancel_lookup(slot, slot);
    for (size_t i = 0; i < SIZED_ICON_MAX; i++) {
        cancel_lookup(slot, &slot->sized[i]);
    }
}

static void cancel_lookup(icon_slot *slot, void *context)
{
    const compositor_services *services = slot->server->services;
    services->cancel_icon_lookups(services->self, context);
}
