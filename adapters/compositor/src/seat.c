#include <math.h>
#include <stdlib.h>
#include <wlr/backend.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_idle_notify_v1.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/util/log.h>

#include "absolute_input.h"
#include "clock.h"
#include "input_mapping.h"
#include "listener.h"
#include "lock.h"
#include "pointer_prefs_compare.h"
#include "seat_private.h"

typedef struct input_device {
    seat *seat;
    struct wlr_input_device *device;
    pointer_prefs applied_prefs;
    bool configured;
    struct wl_list link;
    struct wl_listener destroy;
} input_device;

static bool seat_create_modules(seat *seat);
static void seat_destroy_modules(seat *seat);
static void destroy_pointer_modules(seat *seat);
static void handle_new_input(struct wl_listener *listener, void *data);
static void handle_device_destroy(struct wl_listener *listener, void *data);
static void add_device(seat *seat, struct wlr_input_device *device);
static input_device *track_device(seat *seat, struct wlr_input_device *device);
static void free_device(input_device *device);
static void configure_pointer(input_device *device);
static bool takes_pointer_prefs(const struct wlr_input_device *device);
static void update_capabilities(seat *seat);
static void handle_warp_motion(void *data);

seat *seat_create(compositor *server)
{
    seat *s = calloc(1, sizeof(*s));
    if (s == NULL) {
        return NULL;
    }
    s->server = server;
    wl_list_init(&s->devices);
    wl_list_init(&s->new_input.link);
    s->wlr_seat = wlr_seat_create(server->display, "seat0");
    s->cursor = wlr_cursor_create();
    if (s->wlr_seat == NULL || s->cursor == NULL || !seat_create_modules(s)) {
        wlr_log(WLR_ERROR, "cannot create the seat");
        seat_destroy(s);
        return NULL;
    }
    wlr_cursor_attach_output_layout(s->cursor, server->layout);
    listener_connect(&s->new_input, &server->backend->events.new_input, handle_new_input);
    update_capabilities(s);
    return s;
}

void seat_destroy(seat *s)
{
    listener_disconnect(&s->new_input);
    input_device *device;
    input_device *tmp;
    wl_list_for_each_safe(device, tmp, &s->devices, link) {
        free_device(device);
    }
    seat_destroy_modules(s);
    if (s->warp_motion != NULL) {
        wl_event_source_remove(s->warp_motion);
    }
    if (s->cursor != NULL) {
        wlr_cursor_destroy(s->cursor);
    }
    free(s);
}

struct wlr_seat *seat_wlr(const seat *s)
{
    return s->wlr_seat;
}

void seat_track_device(seat *s, struct wlr_input_device *device)
{
    track_device(s, device);
}

void seat_reload_prefs(seat *s)
{
    keyboards_reload(s->keyboards);
    input_device *device;
    wl_list_for_each(device, &s->devices, link) {
        if (takes_pointer_prefs(device->device)) {
            configure_pointer(device);
        }
    }
    cursor_image_reload(s->image);
}

input_route seat_input_route(const seat *s)
{
    session_lock *lock = s->server->lock;
    if (lock != NULL && session_lock_is_locked(lock)) {
        return INPUT_ROUTE_LOCKED;
    }
    return s->wlr_seat->drag != NULL ? INPUT_ROUTE_DRAG : INPUT_ROUTE_NORMAL;
}

void seat_set_core_cursor(seat *s, wm_cursor_shape shape)
{
    cursor_image_set_core_shape(s->image, shape);
}

void seat_warp(seat *s, wm_point point)
{
    wlr_cursor_warp_closest(s->cursor, NULL, point.x, point.y);
    if (s->warp_motion == NULL) {
        s->warp_motion = wl_event_loop_add_idle(s->server->loop, handle_warp_motion, s);
    }
}

void seat_release_pointer_grab(seat *s)
{
    pointer_input_release_grab(s->pointer);
}

wm_point seat_pointer_position(const seat *s)
{
    return (wm_point){(int) floor(s->cursor->x), (int) floor(s->cursor->y)};
}

uint32_t seat_modifiers(const seat *s)
{
    struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(s->wlr_seat);
    if (keyboard == NULL) {
        return 0;
    }
    return input_mapping_modifiers(wlr_keyboard_get_modifiers(keyboard));
}

void seat_notify_activity(seat *s)
{
    if (s->server->globals.idle_notifier != NULL) {
        wlr_idle_notifier_v1_notify_activity(s->server->globals.idle_notifier, s->wlr_seat);
    }
}

struct wlr_xcursor_manager *seat_xcursors(const seat *s)
{
    return cursor_image_xcursors(s->image);
}

static bool seat_create_modules(seat *s)
{
    s->image = cursor_image_create(s);
    s->keyboards = keyboards_create(s);
    s->shortcuts = shortcuts_create(s);
    s->pointer = pointer_input_create(s);
    s->touch = touch_input_create(s);
    s->absolute = absolute_input_create(&(absolute_places){s->cursor, s->server->layout});
    s->tablet = tablet_input_create(s);
    s->gestures = gestures_create(s);
    s->constraints = constraints_create(s);
    s->selection = selection_create(s);
    s->virtual_input = virtual_input_create(s);
    return s->image != NULL && s->keyboards != NULL && s->shortcuts != NULL &&
           s->pointer != NULL && s->touch != NULL && s->tablet != NULL && s->absolute != NULL &&
           s->gestures != NULL && s->constraints != NULL && s->selection != NULL &&
           s->virtual_input != NULL;
}

static void seat_destroy_modules(seat *s)
{
    destroy_pointer_modules(s);
    if (s->shortcuts != NULL) {
        shortcuts_destroy(s->shortcuts);
    }
    if (s->keyboards != NULL) {
        keyboards_destroy(s->keyboards);
    }
    if (s->image != NULL) {
        cursor_image_destroy(s->image);
    }
}

static void destroy_pointer_modules(seat *s)
{
    if (s->virtual_input != NULL) {
        virtual_input_destroy(s->virtual_input);
    }
    if (s->selection != NULL) {
        selection_destroy(s->selection);
    }
    if (s->constraints != NULL) {
        constraints_destroy(s->constraints);
    }
    if (s->gestures != NULL) {
        gestures_destroy(s->gestures);
    }
    if (s->tablet != NULL) {
        tablet_input_destroy(s->tablet);
    }
    if (s->absolute != NULL) {
        absolute_input_destroy(s->absolute);
    }
    if (s->touch != NULL) {
        touch_input_destroy(s->touch);
    }
    if (s->pointer != NULL) {
        pointer_input_destroy(s->pointer);
    }
}

static void handle_new_input(struct wl_listener *listener, void *data)
{
    seat *s = wl_container_of(listener, s, new_input);
    add_device(s, data);
}

static void handle_device_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    input_device *device = wl_container_of(listener, device, destroy);
    seat *s = device->seat;
    free_device(device);
    update_capabilities(s);
}

static void add_device(seat *s, struct wlr_input_device *device)
{
    switch (device->type) {
    case WLR_INPUT_DEVICE_KEYBOARD:
        keyboards_add(s->keyboards, wlr_keyboard_from_input_device(device));
        break;
    case WLR_INPUT_DEVICE_POINTER:
    case WLR_INPUT_DEVICE_TOUCH:
    case WLR_INPUT_DEVICE_TABLET:
        wlr_cursor_attach_input_device(s->cursor, device);
        break;
    default:
        return;
    }
    input_device *tracked = track_device(s, device);
    if (tracked != NULL && takes_pointer_prefs(device)) {
        configure_pointer(tracked);
    }
}

static input_device *track_device(seat *s, struct wlr_input_device *device)
{
    input_device *tracked = calloc(1, sizeof(*tracked));
    if (tracked == NULL) {
        return NULL;
    }
    tracked->seat = s;
    tracked->device = device;
    listener_connect(&tracked->destroy, &device->events.destroy, handle_device_destroy);
    wl_list_insert(&s->devices, &tracked->link);
    update_capabilities(s);
    return tracked;
}

static void free_device(input_device *device)
{
    listener_disconnect(&device->destroy);
    wl_list_remove(&device->link);
    free(device);
}

static void configure_pointer(input_device *device)
{
    const compositor_services *services = device->seat->server->services;
    pointer_prefs prefs = services->pointer_prefs(services->self, device->device->name);
    if (device->configured && pointer_prefs_equal(&prefs, &device->applied_prefs)) {
        return;
    }
    device->applied_prefs = prefs;
    device->configured = true;
    libinput_config_apply(device->device, &prefs);
    absolute_input_configure(device->seat->absolute, device->device, &prefs.absolute);
}

static bool takes_pointer_prefs(const struct wlr_input_device *device)
{
    return device->type == WLR_INPUT_DEVICE_POINTER || device->type == WLR_INPUT_DEVICE_TOUCH ||
           device->type == WLR_INPUT_DEVICE_TABLET;
}

static void update_capabilities(seat *s)
{
    uint32_t caps = WL_SEAT_CAPABILITY_POINTER;
    input_device *device;
    wl_list_for_each(device, &s->devices, link) {
        if (device->device->type == WLR_INPUT_DEVICE_KEYBOARD) {
            caps |= WL_SEAT_CAPABILITY_KEYBOARD;
        } else if (device->device->type == WLR_INPUT_DEVICE_TOUCH) {
            caps |= WL_SEAT_CAPABILITY_TOUCH;
        }
    }
    wlr_seat_set_capabilities(s->wlr_seat, caps);
}

static void handle_warp_motion(void *data)
{
    seat *s = data;
    s->warp_motion = NULL;
    pointer_input_motion(s->pointer, (uint32_t) clock_now_ms());
}
