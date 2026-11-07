#ifndef COMPOSITOR_XWAYLAND_KEYS_H
#define COMPOSITOR_XWAYLAND_KEYS_H

#include <stdbool.h>

struct wl_client;
struct wlr_keyboard;
struct wlr_keyboard_key_event;
struct wlr_seat;

typedef struct xwayland_keys_route {
    struct wlr_seat *seat;
    struct wl_client *xwayland;
    const struct wlr_keyboard *keyboard;
} xwayland_keys_route;

bool xwayland_keys_send(const xwayland_keys_route *route,
                        const struct wlr_keyboard_key_event *event);
bool xwayland_keys_send_modifiers(const xwayland_keys_route *route);

#endif
