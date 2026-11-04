#include "xfce_output_heads.h"

#include <stdlib.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_management_v1.h>

#include "output.h"
#include "output_head.h"
#include "xfce-output-management-private-v1-protocol.h"

enum {
    MANAGER_VERSION = 1,
};

struct xfce_output_heads {
    compositor *server;
    struct wl_global *global;
    struct wl_list managers;
    struct wl_list heads;
    uint32_t sent_serial;
};

static void bind_manager(struct wl_client *client, void *data, uint32_t version, uint32_t id);
static void handle_stop(struct wl_client *client, struct wl_resource *manager);
static void handle_manager_destroy(struct wl_resource *manager);
static bool drop_missing(xfce_output_heads *heads);
static bool add_new(xfce_output_heads *heads);
static bool is_known(const xfce_output_heads *heads, const struct wlr_output *wlr_output);
static bool has_head(const xfce_output_heads *heads, const struct wlr_output *wlr_output);
static uint32_t current_serial(const xfce_output_heads *heads);
static void send_done(const xfce_output_heads *heads);

static const struct xfce_output_management_manager_private_v1_interface manager_implementation = {
    .stop = handle_stop,
};

xfce_output_heads *xfce_output_heads_create(compositor *server)
{
    xfce_output_heads *xh = calloc(1, sizeof(*xh));
    if (xh == NULL) {
        return NULL;
    }
    xh->server = server;
    wl_list_init(&xh->managers);
    wl_list_init(&xh->heads);
    xh->global = wl_global_create(server->display,
                                  &xfce_output_management_manager_private_v1_interface,
                                  MANAGER_VERSION, xh, bind_manager);
    if (xh->global == NULL) {
        free(xh);
        return NULL;
    }
    return xh;
}

void xfce_output_heads_destroy(xfce_output_heads *xh)
{
    output_head *head;
    output_head *tmp;
    wl_list_for_each_safe(head, tmp, &xh->heads, link) {
        output_head_destroy(head);
    }
    struct wl_resource *manager;
    struct wl_resource *next;
    wl_resource_for_each_safe(manager, next, &xh->managers) {
        wl_resource_set_destructor(manager, NULL);
        wl_resource_set_user_data(manager, NULL);
        wl_list_remove(wl_resource_get_link(manager));
        wl_list_init(wl_resource_get_link(manager));
    }
    wl_global_destroy(xh->global);
    free(xh);
}

void xfce_output_heads_sync(xfce_output_heads *xh)
{
    if (xh == NULL) {
        return;
    }
    bool changed = drop_missing(xh);
    changed = add_new(xh) || changed;
    if (changed || current_serial(xh) != xh->sent_serial) {
        xh->sent_serial = current_serial(xh);
        send_done(xh);
    }
}

bool xfce_output_heads_has_client(const xfce_output_heads *xh, const struct wl_client *client)
{
    struct wl_resource *manager;
    wl_resource_for_each(manager, &xh->managers) {
        if (client != NULL && wl_resource_get_client(manager) == client) {
            return true;
        }
    }
    return false;
}

static void bind_manager(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    xfce_output_heads *xh = data;
    struct wl_resource *manager = wl_resource_create(
        client, &xfce_output_management_manager_private_v1_interface, (int) version, id);
    if (manager == NULL) {
        wl_client_post_no_memory(client);
        return;
    }
    wl_resource_set_implementation(manager, &manager_implementation, xh, handle_manager_destroy);
    wl_list_insert(&xh->managers, wl_resource_get_link(manager));
    output_head *head;
    wl_list_for_each(head, &xh->heads, link) {
        output_head_announce(head, manager);
    }
    xfce_output_management_manager_private_v1_send_done(manager, current_serial(xh));
}

static void handle_stop(struct wl_client *client, struct wl_resource *manager)
{
    (void) client;
    xfce_output_management_manager_private_v1_send_finished(manager);
    wl_resource_destroy(manager);
}

static void handle_manager_destroy(struct wl_resource *manager)
{
    wl_list_remove(wl_resource_get_link(manager));
}

static bool drop_missing(xfce_output_heads *xh)
{
    bool dropped = false;
    output_head *head;
    output_head *tmp;
    wl_list_for_each_safe(head, tmp, &xh->heads, link) {
        if (head->wlr_output == NULL || !is_known(xh, head->wlr_output)) {
            output_head_destroy(head);
            dropped = true;
        }
    }
    return dropped;
}

static bool add_new(xfce_output_heads *xh)
{
    bool added = false;
    size_t count = outputs_known_count(xh->server->outputs);
    for (size_t i = 0; i < count; i++) {
        output *o = outputs_known_at(xh->server->outputs, i);
        if (has_head(xh, output_wlr(o))) {
            continue;
        }
        output_head *head = output_head_create(o, &xh->heads);
        if (head == NULL) {
            continue;
        }
        struct wl_resource *manager;
        wl_resource_for_each(manager, &xh->managers) {
            output_head_announce(head, manager);
        }
        added = true;
    }
    return added;
}

static bool is_known(const xfce_output_heads *xh, const struct wlr_output *wlr_output)
{
    size_t count = outputs_known_count(xh->server->outputs);
    for (size_t i = 0; i < count; i++) {
        if (output_wlr(outputs_known_at(xh->server->outputs, i)) == wlr_output) {
            return true;
        }
    }
    return false;
}

static bool has_head(const xfce_output_heads *xh, const struct wlr_output *wlr_output)
{
    output_head *head;
    wl_list_for_each(head, &xh->heads, link) {
        if (head->wlr_output == wlr_output) {
            return true;
        }
    }
    return false;
}

static uint32_t current_serial(const xfce_output_heads *xh)
{
    struct wlr_output_manager_v1 *manager = xh->server->globals.output_manager;
    return manager != NULL ? manager->serial : 0;
}

static void send_done(const xfce_output_heads *xh)
{
    struct wl_resource *manager;
    wl_resource_for_each(manager, &xh->managers) {
        xfce_output_management_manager_private_v1_send_done(manager, current_serial(xh));
    }
}
