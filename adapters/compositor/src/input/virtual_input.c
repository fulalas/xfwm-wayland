#include <stdlib.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_virtual_keyboard_v1.h>
#include <wlr/types/wlr_virtual_pointer_v1.h>

#include "util/listener.h"
#include "seat_private.h"

struct virtual_input {
    seat *seat;
    struct wl_list mappings;
    struct wl_listener new_keyboard;
    struct wl_listener new_pointer;
};

typedef struct pointer_mapping {
    struct wlr_cursor *cursor;
    struct wlr_input_device *device;
    struct wl_list link;
    struct wl_listener output_destroy;
    struct wl_listener device_destroy;
} pointer_mapping;

static void handle_new_keyboard(struct wl_listener *listener, void *data);
static void handle_new_pointer(struct wl_listener *listener, void *data);
static void map_to_output(virtual_input *virtual_input, struct wlr_input_device *device,
                          struct wlr_output *output);
static void handle_output_destroy(struct wl_listener *listener, void *data);
static void handle_mapped_device_destroy(struct wl_listener *listener, void *data);
static void free_mapping(pointer_mapping *mapping);

virtual_input *virtual_input_create(seat *s)
{
    virtual_input *vi = calloc(1, sizeof(*vi));
    if (vi == NULL) {
        return NULL;
    }
    vi->seat = s;
    wl_list_init(&vi->mappings);
    wl_list_init(&vi->new_keyboard.link);
    wl_list_init(&vi->new_pointer.link);
    protocol_globals *globals = &s->server->globals;
    if (globals->virtual_keyboard != NULL) {
        listener_connect(&vi->new_keyboard, &globals->virtual_keyboard->events.new_virtual_keyboard,
                         handle_new_keyboard);
    }
    if (globals->virtual_pointer != NULL) {
        listener_connect(&vi->new_pointer, &globals->virtual_pointer->events.new_virtual_pointer,
                         handle_new_pointer);
    }
    return vi;
}

void virtual_input_destroy(virtual_input *vi)
{
    listener_disconnect(&vi->new_keyboard);
    listener_disconnect(&vi->new_pointer);
    pointer_mapping *mapping;
    pointer_mapping *tmp;
    wl_list_for_each_safe(mapping, tmp, &vi->mappings, link) {
        free_mapping(mapping);
    }
    free(vi);
}

static void handle_new_keyboard(struct wl_listener *listener, void *data)
{
    virtual_input *vi = wl_container_of(listener, vi, new_keyboard);
    struct wlr_virtual_keyboard_v1 *keyboard = data;
    keyboards_add_virtual(vi->seat->keyboards, &keyboard->keyboard);
    seat_track_device(vi->seat, &keyboard->keyboard.base);
}

static void handle_new_pointer(struct wl_listener *listener, void *data)
{
    virtual_input *vi = wl_container_of(listener, vi, new_pointer);
    const struct wlr_virtual_pointer_v1_new_pointer_event *event = data;
    struct wlr_input_device *device = &event->new_pointer->pointer.base;
    wlr_cursor_attach_input_device(vi->seat->cursor, device);
    if (event->suggested_output != NULL) {
        map_to_output(vi, device, event->suggested_output);
    }
    seat_track_device(vi->seat, device);
}

static void map_to_output(virtual_input *vi, struct wlr_input_device *device,
                          struct wlr_output *output)
{
    pointer_mapping *mapping = calloc(1, sizeof(*mapping));
    if (mapping == NULL) {
        return;
    }
    mapping->cursor = vi->seat->cursor;
    mapping->device = device;
    wl_list_insert(&vi->mappings, &mapping->link);
    listener_connect(&mapping->output_destroy, &output->events.destroy, handle_output_destroy);
    listener_connect(&mapping->device_destroy, &device->events.destroy,
                     handle_mapped_device_destroy);
    wlr_cursor_map_input_to_output(mapping->cursor, device, output);
}

static void handle_output_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    pointer_mapping *mapping = wl_container_of(listener, mapping, output_destroy);
    wlr_cursor_map_input_to_output(mapping->cursor, mapping->device, NULL);
    free_mapping(mapping);
}

static void handle_mapped_device_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    pointer_mapping *mapping = wl_container_of(listener, mapping, device_destroy);
    free_mapping(mapping);
}

static void free_mapping(pointer_mapping *mapping)
{
    listener_disconnect(&mapping->output_destroy);
    listener_disconnect(&mapping->device_destroy);
    wl_list_remove(&mapping->link);
    free(mapping);
}
