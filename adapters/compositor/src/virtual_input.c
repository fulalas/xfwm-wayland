#include <stdlib.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_virtual_keyboard_v1.h>
#include <wlr/types/wlr_virtual_pointer_v1.h>

#include "listener.h"
#include "seat_private.h"

struct virtual_input {
    seat *seat;
    struct wl_listener new_keyboard;
    struct wl_listener new_pointer;
};

static void handle_new_keyboard(struct wl_listener *listener, void *data);
static void handle_new_pointer(struct wl_listener *listener, void *data);

virtual_input *virtual_input_create(seat *s)
{
    virtual_input *vi = calloc(1, sizeof(*vi));
    if (vi == NULL) {
        return NULL;
    }
    vi->seat = s;
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
        wlr_cursor_map_input_to_output(vi->seat->cursor, device, event->suggested_output);
    }
    seat_track_device(vi->seat, device);
}
