#include <stdio.h>
#include <stdlib.h>
#include <wlr/types/wlr_compositor.h>

#include "activation.h"
#include "activation_protocol.h"
#include "util/listener.h"
#include "view.h"

typedef struct pending_activation {
    activation *owner;
    struct wlr_surface *surface;
    bool trusted;
    char launch[WM_LAUNCH_ID_MAX];
    struct wl_listener surface_destroy;
    struct wl_list link;
} pending_activation;

struct activation {
    compositor *server;
    activation_protocol *protocol;
    struct wl_list pending;
    struct wl_listener mapped;
};

static void handle_token_issued(void *self, const char *name);
static void handle_request_activate(void *self, const activation_request *request);
static void handle_mapped(struct wl_listener *listener, void *data);
static void handle_surface_destroy(struct wl_listener *listener, void *data);
static void remember(activation *activation, const activation_request *request);
static pending_activation *find_pending(const activation *activation,
                                        const struct wlr_surface *surface);
static void forget(pending_activation *pending);

activation *activation_create(compositor *server)
{
    activation *a = calloc(1, sizeof(*a));
    if (a == NULL) {
        return NULL;
    }
    a->server = server;
    wl_list_init(&a->pending);
    a->protocol = activation_protocol_create(
        server->display,
        &(activation_request_handler){a, handle_token_issued, handle_request_activate});
    if (a->protocol == NULL) {
        free(a);
        return NULL;
    }
    listener_connect(&a->mapped, &server->view_events.mapped, handle_mapped);
    return a;
}

void activation_destroy(activation *a)
{
    pending_activation *pending;
    pending_activation *tmp;
    wl_list_for_each_safe(pending, tmp, &a->pending, link) {
        forget(pending);
    }
    listener_disconnect(&a->mapped);
    activation_protocol_destroy(a->protocol);
    free(a);
}

bool activation_allows_map(const activation *a, const struct wlr_surface *surface)
{
    const pending_activation *pending = a != NULL ? find_pending(a, surface) : NULL;
    return pending == NULL || pending->trusted;
}

const char *activation_launch_of(const activation *a, const struct wlr_surface *surface)
{
    const pending_activation *pending = a != NULL ? find_pending(a, surface) : NULL;
    return pending != NULL && pending->launch[0] != '\0' ? pending->launch : NULL;
}

static void handle_token_issued(void *self, const char *name)
{
    activation *a = self;
    wm_launch_started(a->server->desktop, name);
}

static void handle_request_activate(void *self, const activation_request *request)
{
    activation *a = self;
    view *v = views_find_by_surface(a->server, request->surface);
    if (v == NULL || view_window(v) == NULL) {
        if (request->trust != ACTIVATION_TOKEN_UNKNOWN) {
            remember(a, request);
        }
        return;
    }
    if (wm_window_launched(a->server->desktop, view_window(v), request->name)) {
        return;
    }
    wm_activation_source source = request->trust == ACTIVATION_TOKEN_TRUSTED
                                      ? WM_ACTIVATION_APPLICATION
                                      : WM_ACTIVATION_APPLICATION_UNTRUSTED;
    wm_request_activate(a->server->desktop, view_window(v), source);
}

static void handle_mapped(struct wl_listener *listener, void *data)
{
    activation *a = wl_container_of(listener, a, mapped);
    pending_activation *pending = find_pending(a, view_surface(data));
    if (pending != NULL) {
        forget(pending);
    }
}

static void handle_surface_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    pending_activation *pending = wl_container_of(listener, pending, surface_destroy);
    forget(pending);
}

static void remember(activation *a, const activation_request *request)
{
    struct wlr_surface *surface = request->surface;
    pending_activation *pending = find_pending(a, surface);
    if (pending == NULL) {
        pending = calloc(1, sizeof(*pending));
        if (pending == NULL) {
            return;
        }
        pending->owner = a;
        pending->surface = surface;
        listener_connect(&pending->surface_destroy, &surface->events.destroy,
                         handle_surface_destroy);
        wl_list_insert(&a->pending, &pending->link);
    }
    pending->trusted = request->trust == ACTIVATION_TOKEN_TRUSTED;
    snprintf(pending->launch, sizeof(pending->launch), "%s", request->name);
}

static pending_activation *find_pending(const activation *a, const struct wlr_surface *surface)
{
    pending_activation *pending;
    wl_list_for_each(pending, &a->pending, link) {
        if (pending->surface == surface) {
            return pending;
        }
    }
    return NULL;
}

static void forget(pending_activation *pending)
{
    listener_disconnect(&pending->surface_destroy);
    wl_list_remove(&pending->link);
    free(pending);
}
