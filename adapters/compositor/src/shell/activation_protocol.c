#include "activation_protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_seat.h>

#include "util/clock.h"
#include "util/listener.h"
#include "util/resource.h"
#include "xdg-activation-v1-protocol.h"

enum {
    TOKEN_BYTES = 16,
    TOKEN_NAME_SIZE = TOKEN_BYTES * 2 + 1,
    TOKEN_LIFETIME_MS = 30000,
    CLIENT_TOKENS_MAX = 16,
    TOKENS_MAX = 64,
};

struct activation_protocol {
    struct wl_global *global;
    activation_request_handler handler;
    struct wl_list issued;
};

typedef struct issued_token {
    char name[TOKEN_NAME_SIZE];
    struct wl_client *client;
    bool trusted;
    int64_t created_ms;
    struct wl_list link;
    struct wl_listener client_destroy;
} issued_token;

typedef struct token_draft {
    activation_protocol *owner;
    struct wl_resource *resource;
    struct wlr_seat *seat;
    uint32_t serial;
    struct wlr_surface *surface;
    bool surface_lost;
    bool committed;
    struct wl_listener seat_destroy;
    struct wl_listener surface_destroy;
} token_draft;

static void bind_activation(struct wl_client *client, void *data, uint32_t version, uint32_t id);
static void handle_get_token(struct wl_client *client, struct wl_resource *resource, uint32_t id);
static void handle_activate(struct wl_client *client, struct wl_resource *resource,
                            const char *name, struct wl_resource *surface);
static void draft_set_serial(struct wl_client *client, struct wl_resource *resource,
                             uint32_t serial, struct wl_resource *seat);
static void draft_set_app_id(struct wl_client *client, struct wl_resource *resource,
                             const char *app_id);
static void draft_set_surface(struct wl_client *client, struct wl_resource *resource,
                              struct wl_resource *surface);
static void draft_commit(struct wl_client *client, struct wl_resource *resource);
static void draft_resource_destroy(struct wl_resource *resource);
static void handle_seat_destroy(struct wl_listener *listener, void *data);
static void handle_surface_destroy(struct wl_listener *listener, void *data);
static token_draft *open_draft(struct wl_resource *resource);
static bool draft_trusted(const token_draft *draft, struct wl_client *client);
static issued_token *issue(activation_protocol *protocol, struct wl_client *client,
                           bool trusted);
static void drop_oldest_of(activation_protocol *protocol, struct wl_client *client);
static void handle_client_destroy(struct wl_listener *listener, void *data);
static issued_token *take_token(activation_protocol *protocol, const char *name);
static void drop_expired(activation_protocol *protocol);
static void drop_token(issued_token *token);
static bool random_name(char *name);

static const struct xdg_activation_v1_interface activation_impl = {
    .destroy = resource_handle_destroy,
    .get_activation_token = handle_get_token,
    .activate = handle_activate,
};

static const struct xdg_activation_token_v1_interface token_impl = {
    .set_serial = draft_set_serial,
    .set_app_id = draft_set_app_id,
    .set_surface = draft_set_surface,
    .commit = draft_commit,
    .destroy = resource_handle_destroy,
};

activation_protocol *activation_protocol_create(struct wl_display *display,
                                                const activation_request_handler *handler)
{
    activation_protocol *protocol = calloc(1, sizeof(*protocol));
    if (protocol == NULL) {
        return NULL;
    }
    protocol->handler = *handler;
    wl_list_init(&protocol->issued);
    protocol->global = wl_global_create(display, &xdg_activation_v1_interface, 1, protocol,
                                        bind_activation);
    if (protocol->global == NULL) {
        free(protocol);
        return NULL;
    }
    return protocol;
}

void activation_protocol_destroy(activation_protocol *protocol)
{
    issued_token *token;
    issued_token *tmp;
    wl_list_for_each_safe(token, tmp, &protocol->issued, link) {
        drop_token(token);
    }
    wl_global_destroy(protocol->global);
    free(protocol);
}

static void bind_activation(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    resource_bind(client, &xdg_activation_v1_interface, version, id, &activation_impl, data, NULL);
}

static void handle_get_token(struct wl_client *client, struct wl_resource *resource, uint32_t id)
{
    token_draft *draft = calloc(1, sizeof(*draft));
    if (draft == NULL) {
        wl_client_post_no_memory(client);
        return;
    }
    draft->owner = wl_resource_get_user_data(resource);
    draft->resource = wl_resource_create(client, &xdg_activation_token_v1_interface,
                                         wl_resource_get_version(resource), id);
    if (draft->resource == NULL) {
        free(draft);
        wl_client_post_no_memory(client);
        return;
    }
    wl_list_init(&draft->seat_destroy.link);
    wl_list_init(&draft->surface_destroy.link);
    wl_resource_set_implementation(draft->resource, &token_impl, draft, draft_resource_destroy);
}

static void handle_activate(struct wl_client *client, struct wl_resource *resource,
                            const char *name, struct wl_resource *surface)
{
    (void) client;
    activation_protocol *protocol = wl_resource_get_user_data(resource);
    drop_expired(protocol);
    issued_token *token = take_token(protocol, name);
    activation_request request = {
        .surface = wlr_surface_from_resource(surface),
        .name = name,
        .trust = ACTIVATION_TOKEN_UNKNOWN,
    };
    if (token != NULL) {
        request.trust = token->trusted ? ACTIVATION_TOKEN_TRUSTED : ACTIVATION_TOKEN_UNTRUSTED;
        drop_token(token);
    }
    protocol->handler.activate(protocol->handler.self, &request);
}

static void draft_set_serial(struct wl_client *client, struct wl_resource *resource,
                             uint32_t serial, struct wl_resource *seat)
{
    (void) client;
    token_draft *draft = open_draft(resource);
    struct wlr_seat_client *seat_client = wlr_seat_client_from_resource(seat);
    if (draft == NULL || seat_client == NULL) {
        return;
    }
    listener_disconnect(&draft->seat_destroy);
    draft->seat = seat_client->seat;
    draft->serial = serial;
    listener_connect(&draft->seat_destroy, &draft->seat->events.destroy, handle_seat_destroy);
}

static void draft_set_app_id(struct wl_client *client, struct wl_resource *resource,
                             const char *app_id)
{
    (void) client;
    (void) app_id;
    open_draft(resource);
}

static void draft_set_surface(struct wl_client *client, struct wl_resource *resource,
                              struct wl_resource *surface)
{
    (void) client;
    token_draft *draft = open_draft(resource);
    if (draft == NULL) {
        return;
    }
    listener_disconnect(&draft->surface_destroy);
    draft->surface = wlr_surface_from_resource(surface);
    draft->surface_lost = false;
    listener_connect(&draft->surface_destroy, &draft->surface->events.destroy,
                     handle_surface_destroy);
}

static void draft_commit(struct wl_client *client, struct wl_resource *resource)
{
    token_draft *draft = open_draft(resource);
    if (draft == NULL) {
        return;
    }
    draft->committed = true;
    drop_expired(draft->owner);
    issued_token *token = issue(draft->owner, client, draft_trusted(draft, client));
    if (token == NULL) {
        wl_client_post_no_memory(client);
        return;
    }
    xdg_activation_token_v1_send_done(resource, token->name);
    draft->owner->handler.issued(draft->owner->handler.self, token->name);
}

static void draft_resource_destroy(struct wl_resource *resource)
{
    token_draft *draft = wl_resource_get_user_data(resource);
    listener_disconnect(&draft->seat_destroy);
    listener_disconnect(&draft->surface_destroy);
    free(draft);
}

static void handle_seat_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    token_draft *draft = wl_container_of(listener, draft, seat_destroy);
    listener_disconnect(&draft->seat_destroy);
    draft->seat = NULL;
}

static void handle_surface_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    token_draft *draft = wl_container_of(listener, draft, surface_destroy);
    listener_disconnect(&draft->surface_destroy);
    draft->surface = NULL;
    draft->surface_lost = true;
}

static token_draft *open_draft(struct wl_resource *resource)
{
    token_draft *draft = wl_resource_get_user_data(resource);
    if (draft->committed) {
        wl_resource_post_error(resource, XDG_ACTIVATION_TOKEN_V1_ERROR_ALREADY_USED,
                               "the activation token has already been used");
        return NULL;
    }
    return draft;
}

static bool draft_trusted(const token_draft *draft, struct wl_client *client)
{
    if (draft->seat == NULL || draft->surface_lost) {
        return false;
    }
    struct wlr_seat_client *seat_client = wlr_seat_client_for_wl_client(draft->seat, client);
    if (seat_client == NULL ||
        !wlr_seat_client_validate_event_serial(seat_client, draft->serial)) {
        return false;
    }
    return draft->surface == NULL || draft->seat->keyboard_state.focused_client == seat_client ||
           draft->seat->pointer_state.focused_client == seat_client;
}

static issued_token *issue(activation_protocol *protocol, struct wl_client *client,
                           bool trusted)
{
    drop_oldest_of(protocol, client);
    if (wl_list_length(&protocol->issued) >= TOKENS_MAX) {
        issued_token *oldest = wl_container_of(protocol->issued.next, oldest, link);
        drop_token(oldest);
    }
    issued_token *token = calloc(1, sizeof(*token));
    if (token == NULL || !random_name(token->name)) {
        free(token);
        return NULL;
    }
    token->client = client;
    token->trusted = trusted;
    token->created_ms = clock_now_ms();
    wl_list_insert(protocol->issued.prev, &token->link);
    token->client_destroy.notify = handle_client_destroy;
    wl_client_add_destroy_listener(client, &token->client_destroy);
    return token;
}

static void drop_oldest_of(activation_protocol *protocol, struct wl_client *client)
{
    issued_token *oldest = NULL;
    size_t count = 0;
    issued_token *token;
    wl_list_for_each(token, &protocol->issued, link) {
        if (token->client == client) {
            oldest = oldest != NULL ? oldest : token;
            count++;
        }
    }
    if (count >= CLIENT_TOKENS_MAX) {
        drop_token(oldest);
    }
}

static issued_token *take_token(activation_protocol *protocol, const char *name)
{
    issued_token *token;
    wl_list_for_each(token, &protocol->issued, link) {
        if (strcmp(token->name, name) == 0) {
            return token;
        }
    }
    return NULL;
}

static void drop_expired(activation_protocol *protocol)
{
    int64_t now = clock_now_ms();
    issued_token *token;
    issued_token *tmp;
    wl_list_for_each_safe(token, tmp, &protocol->issued, link) {
        if (now - token->created_ms > TOKEN_LIFETIME_MS) {
            drop_token(token);
        }
    }
}

static void drop_token(issued_token *token)
{
    wl_list_remove(&token->client_destroy.link);
    wl_list_remove(&token->link);
    free(token);
}

static void handle_client_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    issued_token *token = wl_container_of(listener, token, client_destroy);
    listener_disconnect(&token->client_destroy);
    token->client = NULL;
}

static bool random_name(char *name)
{
    unsigned char bytes[TOKEN_BYTES];
    if (getrandom(bytes, sizeof(bytes), 0) != (ssize_t) sizeof(bytes)) {
        return false;
    }
    for (size_t i = 0; i < sizeof(bytes); i++) {
        snprintf(name + i * 2, 3, "%02x", bytes[i]);
    }
    return true;
}
