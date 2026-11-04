#include "xfce_outputs.h"

#include <stdlib.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_seat.h>

#include "listener.h"
#include "output.h"
#include "output_record.h"
#include "seat_private.h"
#include "xfce-output-private-v1-protocol.h"

enum {
    MANAGER_VERSION = 1,
};

struct xfce_outputs {
    compositor *server;
    struct wl_global *global;
    struct wl_list records;
    struct wl_listener cursor_frame;
    struct wl_listener focus_change;
};

static void bind_manager(struct wl_client *client, void *data, uint32_t version, uint32_t id);
static void handle_get_output(struct wl_client *client, struct wl_resource *manager,
                              uint32_t id, struct wl_resource *wl_output);
static void handle_destroy_request(struct wl_client *client, struct wl_resource *resource);
static void handle_cursor_frame(struct wl_listener *listener, void *data);
static void handle_focus_change(struct wl_listener *listener, void *data);
static output *output_of_resource(struct wl_resource *wl_output);
static void update_pointer(xfce_outputs *outputs);

static const struct xfce_output_manager_private_v1_interface manager_implementation = {
    .get_xfce_output = handle_get_output,
    .release = handle_destroy_request,
};

static const struct xfce_output_v1_interface output_implementation = {
    .destroy = handle_destroy_request,
};

xfce_outputs *xfce_outputs_create(compositor *server)
{
    xfce_outputs *xo = calloc(1, sizeof(*xo));
    if (xo == NULL) {
        return NULL;
    }
    xo->server = server;
    wl_list_init(&xo->records);
    xo->global = wl_global_create(server->display, &xfce_output_manager_private_v1_interface,
                                  MANAGER_VERSION, xo, bind_manager);
    if (xo->global == NULL) {
        free(xo);
        return NULL;
    }
    struct wlr_seat *wlr_seat = seat_wlr(server->seat);
    listener_connect(&xo->cursor_frame, &server->seat->cursor->events.frame, handle_cursor_frame);
    listener_connect(&xo->focus_change, &wlr_seat->pointer_state.events.focus_change,
                     handle_focus_change);
    return xo;
}

void xfce_outputs_destroy(xfce_outputs *xo)
{
    output_record *record;
    output_record *tmp;
    wl_list_for_each_safe(record, tmp, &xo->records, link) {
        output_record_orphan(record);
    }
    listener_disconnect(&xo->cursor_frame);
    listener_disconnect(&xo->focus_change);
    wl_global_destroy(xo->global);
    free(xo);
}

void xfce_outputs_refresh(xfce_outputs *xo)
{
    if (xo == NULL) {
        return;
    }
    output_record *record;
    wl_list_for_each(record, &xo->records, link) {
        output_record_send_changes(record);
    }
    update_pointer(xo);
}

static void bind_manager(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    struct wl_resource *resource =
        wl_resource_create(client, &xfce_output_manager_private_v1_interface, (int) version, id);
    if (resource == NULL) {
        wl_client_post_no_memory(client);
        return;
    }
    wl_resource_set_implementation(resource, &manager_implementation, data, NULL);
}

static void handle_get_output(struct wl_client *client, struct wl_resource *manager,
                              uint32_t id, struct wl_resource *wl_output)
{
    xfce_outputs *xo = wl_resource_get_user_data(manager);
    struct wl_resource *resource = wl_resource_create(
        client, &xfce_output_v1_interface, wl_resource_get_version(manager), id);
    if (resource == NULL) {
        wl_client_post_no_memory(client);
        return;
    }
    wl_resource_set_implementation(resource, &output_implementation, NULL, NULL);
    output_record *record = xo != NULL ? output_record_create(resource, &xo->records) : NULL;
    if (record == NULL) {
        xfce_output_v1_send_done(resource);
        return;
    }
    output_record_attach(record, output_of_resource(wl_output));
    update_pointer(xo);
}

static void handle_destroy_request(struct wl_client *client, struct wl_resource *resource)
{
    (void) client;
    wl_resource_destroy(resource);
}

static void handle_cursor_frame(struct wl_listener *listener, void *data)
{
    (void) data;
    xfce_outputs *xo = wl_container_of(listener, xo, cursor_frame);
    update_pointer(xo);
}

static void handle_focus_change(struct wl_listener *listener, void *data)
{
    (void) data;
    xfce_outputs *xo = wl_container_of(listener, xo, focus_change);
    update_pointer(xo);
}

static output *output_of_resource(struct wl_resource *wl_output)
{
    struct wlr_output *wlr_output = wlr_output_from_resource(wl_output);
    return wlr_output != NULL ? output_from_wlr(wlr_output) : NULL;
}

static void update_pointer(xfce_outputs *xo)
{
    output *target = outputs_at_point(xo->server->outputs, seat_pointer_position(xo->server->seat));
    struct wlr_seat *wlr_seat = seat_wlr(xo->server->seat);
    output_record *record;
    wl_list_for_each(record, &xo->records, link) {
        output_record_leave_unless(record, target, wlr_seat);
    }
    wl_list_for_each(record, &xo->records, link) {
        output_record_enter_if(record, target, wlr_seat);
    }
}
