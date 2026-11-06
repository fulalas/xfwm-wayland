#ifndef COMPOSITOR_ACTIVATION_PROTOCOL_H
#define COMPOSITOR_ACTIVATION_PROTOCOL_H

#include <stdbool.h>
#include <wayland-server-core.h>

struct wlr_surface;

typedef struct activation_protocol activation_protocol;

typedef struct activation_request_handler {
    void *self;
    void (*activate)(void *self, struct wlr_surface *surface, bool trusted);
} activation_request_handler;

activation_protocol *activation_protocol_create(struct wl_display *display,
                                                const activation_request_handler *handler);
void activation_protocol_destroy(activation_protocol *protocol);

#endif
