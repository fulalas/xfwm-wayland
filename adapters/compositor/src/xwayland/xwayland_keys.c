#include "xwayland_keys.h"

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_seat.h>

static struct wlr_seat_client *unfocused_client(struct wlr_seat *seat, struct wl_client *xwayland);
static void send_modifiers(struct wlr_seat_client *client, const struct wlr_keyboard *keyboard,
                           uint32_t serial);

bool xwayland_keys_send(const xwayland_keys_route *route,
                        const struct wlr_keyboard_key_event *event)
{
    struct wlr_seat_client *client = unfocused_client(route->seat, route->xwayland);
    if (client == NULL) {
        return false;
    }
    uint32_t serial = wlr_seat_client_next_serial(client);
    send_modifiers(client, route->keyboard, serial);
    struct wl_resource *resource;
    wl_resource_for_each(resource, &client->keyboards) {
        wl_keyboard_send_key(resource, serial, event->time_msec, event->keycode, event->state);
    }
    return true;
}

bool xwayland_keys_send_modifiers(const xwayland_keys_route *route)
{
    struct wlr_seat_client *client = unfocused_client(route->seat, route->xwayland);
    if (client == NULL) {
        return false;
    }
    send_modifiers(client, route->keyboard, wlr_seat_client_next_serial(client));
    return true;
}

static struct wlr_seat_client *unfocused_client(struct wlr_seat *seat, struct wl_client *xwayland)
{
    struct wlr_seat_client *client =
        xwayland != NULL ? wlr_seat_client_for_wl_client(seat, xwayland) : NULL;
    return client != NULL && seat->keyboard_state.focused_client != client ? client : NULL;
}

static void send_modifiers(struct wlr_seat_client *client, const struct wlr_keyboard *keyboard,
                           uint32_t serial)
{
    const struct wlr_keyboard_modifiers *mods = &keyboard->modifiers;
    struct wl_resource *resource;
    wl_resource_for_each(resource, &client->keyboards) {
        wl_keyboard_send_modifiers(resource, serial, mods->depressed, mods->latched,
                                   mods->locked, mods->group);
    }
}
