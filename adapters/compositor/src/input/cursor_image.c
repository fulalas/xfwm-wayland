#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_cursor_shape_v1.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/util/log.h>

#include "cursor/cursor_names.h"
#include "cursor/cursor_owner.h"
#include "cursor/cursor_sprite.h"
#include "cursor/xcursor_cache.h"
#include "util/listener.h"
#include "output/output.h"
#include "seat_private.h"

enum {
    DEFAULT_CURSOR_SIZE = 24,
    SIZE_TEXT_MAX = 16,
};

struct cursor_image {
    seat *seat;
    cursor_prefs prefs;
    xcursor_cache *xcursors;
    scalable_theme *scalable;
    scalable_theme_request scalable_key;
    cursor_sprite *sprite;
    wm_cursor_shape core_shape;
    const char *client_shape;
    bool client_mode;
    bool busy;
    bool showing_busy;
    cursor_surface client;
    struct wl_listener client_surface_destroy;
    struct wl_listener request_set_cursor;
    struct wl_listener request_set_shape;
};

static void connect_requests(cursor_image *image);
static cursor_prefs current_prefs(const seat *seat);
static bool same_prefs(const cursor_prefs *a, const cursor_prefs *b);
static xcursor_cache *load_theme(const cursor_prefs *prefs);
static int theme_size(const cursor_prefs *prefs);
static cursor_theme_view theme_view(cursor_image *image);
static void refresh_scalable(cursor_image *image, int size, double scale);
static void close_scalable(cursor_image *image);
static void apply_current(cursor_image *image);
static void apply_core_shape(cursor_image *image);
static void apply_shape(cursor_image *image, wm_cursor_shape shape);
static void apply_client(cursor_image *image);
static bool busy_over_client(const cursor_image *image);
static void remember_surface(cursor_image *image, const cursor_surface *surface);
static void handle_client_surface_destroy(struct wl_listener *listener, void *data);
static void apply_client_shape(cursor_image *image);
static void handle_request_set_cursor(struct wl_listener *listener, void *data);
static void handle_request_set_shape(struct wl_listener *listener, void *data);
static bool accepts_request(const cursor_image *image, struct wlr_seat_client *client);
static bool drag_active(const cursor_image *image);

cursor_image *cursor_image_create(seat *s)
{
    cursor_image *image = calloc(1, sizeof(*image));
    if (image == NULL) {
        return NULL;
    }
    image->seat = s;
    image->sprite = cursor_sprite_create(s->cursor, s->server->loop);
    if (image->sprite == NULL) {
        free(image);
        return NULL;
    }
    image->prefs = current_prefs(s);
    image->xcursors = load_theme(&image->prefs);
    connect_requests(image);
    apply_core_shape(image);
    return image;
}

static void connect_requests(cursor_image *image)
{
    seat *s = image->seat;
    wl_list_init(&image->request_set_shape.link);
    wl_list_init(&image->client_surface_destroy.link);
    listener_connect(&image->request_set_cursor, &s->wlr_seat->events.request_set_cursor,
                     handle_request_set_cursor);
    struct wlr_cursor_shape_manager_v1 *shapes = s->server->globals.cursor_shape;
    if (shapes != NULL) {
        listener_connect(&image->request_set_shape, &shapes->events.request_set_shape,
                         handle_request_set_shape);
    }
}

void cursor_image_destroy(cursor_image *image)
{
    listener_disconnect(&image->request_set_cursor);
    listener_disconnect(&image->request_set_shape);
    listener_disconnect(&image->client_surface_destroy);
    cursor_sprite_destroy(image->sprite);
    close_scalable(image);
    xcursor_cache_destroy(image->xcursors);
    free(image);
}

void cursor_image_reload(cursor_image *image)
{
    cursor_prefs prefs = current_prefs(image->seat);
    if (same_prefs(&prefs, &image->prefs)) {
        return;
    }
    image->prefs = prefs;
    cursor_sprite_stop(image->sprite);
    close_scalable(image);
    xcursor_cache_destroy(image->xcursors);
    image->xcursors = load_theme(&image->prefs);
    image->showing_busy = false;
    apply_current(image);
}

void cursor_image_refresh(cursor_image *image)
{
    image->showing_busy = false;
    apply_current(image);
}

void cursor_image_set_surface(cursor_image *image, const cursor_surface *surface)
{
    image->client_mode = true;
    image->client_shape = NULL;
    remember_surface(image, surface);
    apply_client(image);
}

void cursor_image_set_core_shape(cursor_image *image, wm_cursor_shape shape)
{
    image->core_shape = shape;
    if (!image->client_mode && !drag_active(image)) {
        apply_core_shape(image);
    }
}

void cursor_image_use_core(cursor_image *image)
{
    if (image->client_mode && !drag_active(image)) {
        image->client_mode = false;
        apply_core_shape(image);
    }
}

void cursor_image_set_busy(cursor_image *image, bool busy)
{
    if (image->busy != busy) {
        image->busy = busy;
        if (image->client_mode) {
            apply_client(image);
        }
    }
}

void cursor_image_use_client(cursor_image *image)
{
    image->client_mode = true;
}

xcursor_cache *cursor_image_xcursors(const cursor_image *image)
{
    return image->xcursors;
}

static cursor_prefs current_prefs(const seat *s)
{
    const compositor_services *services = s->server->services;
    return services->cursor_prefs(services->self);
}

static bool same_prefs(const cursor_prefs *a, const cursor_prefs *b)
{
    return a->size == b->size && strcmp(a->theme, b->theme) == 0;
}

static xcursor_cache *load_theme(const cursor_prefs *prefs)
{
    int size = theme_size(prefs);
    const char *theme = prefs->theme[0] != '\0' ? prefs->theme : NULL;
    char size_text[SIZE_TEXT_MAX];
    snprintf(size_text, sizeof(size_text), "%d", size);
    setenv("XCURSOR_SIZE", size_text, 1);
    if (theme != NULL) {
        setenv("XCURSOR_THEME", theme, 1);
    } else {
        unsetenv("XCURSOR_THEME");
    }
    xcursor_cache *cache = xcursor_cache_create(theme, (uint32_t) size);
    if (theme != NULL && !xcursor_cache_has_theme(cache)) {
        wlr_log(WLR_ERROR, "cursor theme %s not found, using the default cursors", theme);
    }
    return cache;
}

static int theme_size(const cursor_prefs *prefs)
{
    return prefs->size > 0 ? prefs->size : DEFAULT_CURSOR_SIZE;
}

static cursor_theme_view theme_view(cursor_image *image)
{
    double scale = outputs_max_scale(image->seat->server->outputs);
    int size = theme_size(&image->prefs);
    refresh_scalable(image, size, scale);
    return (cursor_theme_view) {
        .scalable = image->scalable,
        .xcursors = image->xcursors,
        .scale = (float) scale,
        .size = size,
    };
}

static void refresh_scalable(cursor_image *image, int size, double scale)
{
    scalable_theme_request *key = &image->scalable_key;
    if (key->name != NULL && key->size == size && key->scale == scale) {
        return;
    }
    cursor_sprite_stop(image->sprite);
    close_scalable(image);
    *key = (scalable_theme_request) {image->prefs.theme, size, scale};
    image->scalable = scalable_theme_open(key);
}

static void close_scalable(cursor_image *image)
{
    if (image->scalable != NULL) {
        scalable_theme_close(image->scalable);
        image->scalable = NULL;
    }
    image->scalable_key.name = NULL;
}

static void apply_current(cursor_image *image)
{
    if (image->client_mode) {
        apply_client(image);
    } else if (!drag_active(image)) {
        apply_core_shape(image);
    }
}

static void apply_core_shape(cursor_image *image)
{
    apply_shape(image, image->core_shape);
}

static void apply_client(cursor_image *image)
{
    if (busy_over_client(image)) {
        if (!image->showing_busy) {
            apply_shape(image, WM_CURSOR_BUSY);
        }
        return;
    }
    if (image->client_shape != NULL) {
        apply_client_shape(image);
        return;
    }
    image->showing_busy = false;
    cursor_sprite_stop(image->sprite);
    wlr_cursor_set_surface(image->seat->cursor, image->client.surface, image->client.hotspot_x,
                           image->client.hotspot_y);
}

static bool busy_over_client(const cursor_image *image)
{
    if (!image->busy || drag_active(image)) {
        return false;
    }
    if (image->client_shape != NULL) {
        return strcmp(image->client_shape, "default") == 0;
    }
    return image->client.surface != NULL;
}

static void remember_surface(cursor_image *image, const cursor_surface *surface)
{
    listener_disconnect(&image->client_surface_destroy);
    image->client = *surface;
    if (surface->surface != NULL) {
        listener_connect(&image->client_surface_destroy, &surface->surface->events.destroy,
                         handle_client_surface_destroy);
    }
}

static void handle_client_surface_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    cursor_image *image = wl_container_of(listener, image, client_surface_destroy);
    listener_disconnect(&image->client_surface_destroy);
    image->client.surface = NULL;
}

static void apply_shape(cursor_image *image, wm_cursor_shape shape)
{
    image->showing_busy = shape == WM_CURSOR_BUSY;
    cursor_theme_view view = theme_view(image);
    cursor_name_list names = cursor_names_for_shape(shape);
    for (int i = 0; i < CURSOR_NAME_CHOICES && names.names[i] != NULL; i++) {
        if (cursor_sprite_show(image->sprite, &view, names.names[i])) {
            return;
        }
    }
    cursor_sprite_show(image->sprite, &view, "default");
}

static void handle_request_set_cursor(struct wl_listener *listener, void *data)
{
    cursor_image *image = wl_container_of(listener, image, request_set_cursor);
    const struct wlr_seat_pointer_request_set_cursor_event *event = data;
    if (accepts_request(image, event->seat_client)) {
        cursor_surface surface = {event->surface, event->hotspot_x, event->hotspot_y};
        cursor_image_set_surface(image, &surface);
    }
}

static void handle_request_set_shape(struct wl_listener *listener, void *data)
{
    cursor_image *image = wl_container_of(listener, image, request_set_shape);
    const struct wlr_cursor_shape_manager_v1_request_set_shape_event *event = data;
    if (accepts_request(image, event->seat_client)) {
        image->client_mode = true;
        image->client_shape = wlr_cursor_shape_v1_name(event->shape);
        apply_client(image);
    }
}

static void apply_client_shape(cursor_image *image)
{
    image->showing_busy = false;
    cursor_theme_view view = theme_view(image);
    cursor_sprite_show(image->sprite, &view, image->client_shape);
}

static bool accepts_request(const cursor_image *image, struct wlr_seat_client *client)
{
    const struct wlr_seat *wlr_seat = image->seat->wlr_seat;
    const struct wlr_drag *drag = wlr_seat->drag;
    cursor_request request = {
        .requester = client,
        .pointer_focus = wlr_seat->pointer_state.focused_client,
        .drag_source = drag != NULL ? drag->seat_client : NULL,
        .drag_active = drag_active(image),
        .client_mode = image->client_mode,
    };
    return cursor_owner_accepts(&request);
}

static bool drag_active(const cursor_image *image)
{
    return image->seat->wlr_seat->drag != NULL;
}
