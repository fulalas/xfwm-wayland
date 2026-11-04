#include <stdlib.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xdg_activation_v1.h>

#include "activation.h"
#include "listener.h"
#include "view.h"

typedef struct pending_activation {
    activation *owner;
    struct wlr_surface *surface;
    bool trusted;
    struct wl_listener surface_destroy;
    struct wl_list link;
} pending_activation;

struct activation {
    compositor *server;
    struct wl_list pending;
    struct wl_listener new_token;
    struct wl_listener request_activate;
    struct wl_listener mapped;
};

static char trusted_mark;

static void handle_new_token(struct wl_listener *listener, void *data);
static void handle_request_activate(struct wl_listener *listener, void *data);
static void handle_mapped(struct wl_listener *listener, void *data);
static void handle_surface_destroy(struct wl_listener *listener, void *data);
static bool token_trusted(const struct wlr_xdg_activation_token_v1 *token);
static bool client_has_input_focus(const struct wlr_seat *seat,
                                   const struct wlr_seat_client *client);
static void remember(activation *activation, struct wlr_surface *surface, bool trusted);
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
    wl_list_init(&a->new_token.link);
    wl_list_init(&a->request_activate.link);
    listener_connect(&a->mapped, &server->view_events.mapped, handle_mapped);
    struct wlr_xdg_activation_v1 *manager = server->globals.activation;
    if (manager != NULL) {
        listener_connect(&a->new_token, &manager->events.new_token, handle_new_token);
        listener_connect(&a->request_activate, &manager->events.request_activate,
                         handle_request_activate);
    }
    return a;
}

void activation_destroy(activation *a)
{
    pending_activation *pending;
    pending_activation *tmp;
    wl_list_for_each_safe(pending, tmp, &a->pending, link) {
        forget(pending);
    }
    listener_disconnect(&a->new_token);
    listener_disconnect(&a->request_activate);
    listener_disconnect(&a->mapped);
    free(a);
}

bool activation_allows_map(const activation *a, const struct wlr_surface *surface)
{
    const pending_activation *pending = a != NULL ? find_pending(a, surface) : NULL;
    return pending == NULL || pending->trusted;
}

static void handle_new_token(struct wl_listener *listener, void *data)
{
    (void) listener;
    struct wlr_xdg_activation_token_v1 *token = data;
    token->data = token_trusted(token) ? &trusted_mark : NULL;
}

static void handle_request_activate(struct wl_listener *listener, void *data)
{
    activation *a = wl_container_of(listener, a, request_activate);
    const struct wlr_xdg_activation_v1_request_activate_event *event = data;
    bool trusted = event->token->data == &trusted_mark;
    view *v = views_find_by_surface(a->server, event->surface);
    if (v == NULL || view_window(v) == NULL) {
        remember(a, event->surface, trusted);
        return;
    }
    wm_activation_source source =
        trusted ? WM_ACTIVATION_APPLICATION : WM_ACTIVATION_APPLICATION_UNTRUSTED;
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

static bool token_trusted(const struct wlr_xdg_activation_token_v1 *token)
{
    if (token->seat == NULL) {
        return false;
    }
    if (token->surface == NULL || token->surface->resource == NULL) {
        return true;
    }
    struct wl_client *client = wl_resource_get_client(token->surface->resource);
    struct wlr_seat_client *seat_client = wlr_seat_client_for_wl_client(token->seat, client);
    if (seat_client == NULL || !wlr_seat_client_validate_event_serial(seat_client,
                                                                      token->serial)) {
        return false;
    }
    return client_has_input_focus(token->seat, seat_client);
}

static bool client_has_input_focus(const struct wlr_seat *seat,
                                   const struct wlr_seat_client *client)
{
    return seat->keyboard_state.focused_client == client ||
           seat->pointer_state.focused_client == client;
}

static void remember(activation *a, struct wlr_surface *surface, bool trusted)
{
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
    pending->trusted = trusted;
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
