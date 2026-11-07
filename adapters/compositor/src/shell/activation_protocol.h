#ifndef COMPOSITOR_ACTIVATION_PROTOCOL_H
#define COMPOSITOR_ACTIVATION_PROTOCOL_H

#include <stdbool.h>
#include <wayland-server-core.h>

struct wlr_surface;

typedef struct activation_protocol activation_protocol;

typedef enum activation_trust {
    ACTIVATION_TOKEN_UNKNOWN,
    ACTIVATION_TOKEN_UNTRUSTED,
    ACTIVATION_TOKEN_TRUSTED,
} activation_trust;

typedef struct activation_request {
    struct wlr_surface *surface;
    const char *name;
    activation_trust trust;
} activation_request;

typedef struct activation_request_handler {
    void *self;
    void (*issued)(void *self, const char *name);
    void (*activate)(void *self, const activation_request *request);
} activation_request_handler;

activation_protocol *activation_protocol_create(struct wl_display *display,
                                                const activation_request_handler *handler);
void activation_protocol_destroy(activation_protocol *protocol);

#endif
