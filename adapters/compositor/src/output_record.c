#include "output_record.h"

#include <stdlib.h>
#include <unistd.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_seat.h>

#include "listener.h"
#include "output_setting.h"
#include "xfce-output-private-v1-protocol.h"

typedef void (*pointer_event)(struct wl_resource *resource, struct wl_resource *seat);

static void handle_resource_destroy(struct wl_resource *resource);
static void handle_output_destroy(struct wl_listener *listener, void *data);
static void send_identity(output_record *record);
static bool send_workarea(output_record *record);
static bool send_primary(output_record *record);
static wm_rect relative_workarea(const output *output);
static void send_pointer(output_record *record, struct wlr_seat *seat, pointer_event event);

output_record *output_record_create(struct wl_resource *resource, struct wl_list *records)
{
    output_record *record = calloc(1, sizeof(*record));
    if (record == NULL) {
        return NULL;
    }
    record->resource = resource;
    wl_list_init(&record->output_destroy.link);
    wl_list_insert(records, &record->link);
    wl_resource_set_user_data(resource, record);
    wl_resource_set_destructor(resource, handle_resource_destroy);
    return record;
}

void output_record_attach(output_record *record, output *o)
{
    if (o == NULL) {
        xfce_output_v1_send_done(record->resource);
        return;
    }
    record->output = o;
    listener_connect(&record->output_destroy, &output_wlr(o)->events.destroy,
                     handle_output_destroy);
    send_identity(record);
    send_primary(record);
    record->workarea = relative_workarea(o);
    xfce_output_v1_send_workarea(record->resource, record->workarea.x, record->workarea.y,
                                 (uint32_t) record->workarea.width,
                                 (uint32_t) record->workarea.height);
    xfce_output_v1_send_done(record->resource);
}

void output_record_send_changes(output_record *record)
{
    if (record->output == NULL || !output_is_active(record->output)) {
        return;
    }
    bool workarea_sent = send_workarea(record);
    bool primary_sent = send_primary(record);
    if (workarea_sent || primary_sent) {
        xfce_output_v1_send_done(record->resource);
    }
}

void output_record_leave_unless(output_record *record, output *target, struct wlr_seat *seat)
{
    if (record->pointer_inside && record->output != target) {
        record->pointer_inside = false;
        send_pointer(record, seat, xfce_output_v1_send_pointer_leave);
    }
}

void output_record_enter_if(output_record *record, output *target, struct wlr_seat *seat)
{
    if (!record->pointer_inside && record->output != NULL && record->output == target) {
        record->pointer_inside = true;
        send_pointer(record, seat, xfce_output_v1_send_pointer_enter);
    }
}

void output_record_orphan(output_record *record)
{
    wl_resource_set_user_data(record->resource, NULL);
    wl_resource_set_destructor(record->resource, NULL);
    listener_disconnect(&record->output_destroy);
    wl_list_remove(&record->link);
    free(record);
}

static void handle_resource_destroy(struct wl_resource *resource)
{
    output_record *record = wl_resource_get_user_data(resource);
    if (record == NULL) {
        return;
    }
    listener_disconnect(&record->output_destroy);
    wl_list_remove(&record->link);
    free(record);
}

static void handle_output_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    output_record *record = wl_container_of(listener, record, output_destroy);
    listener_disconnect(&record->output_destroy);
    record->output = NULL;
    record->pointer_inside = false;
}

static void send_identity(output_record *record)
{
    struct wlr_output *wlr_output = output_wlr(record->output);
    if (wlr_output->serial != NULL) {
        xfce_output_v1_send_serial(record->resource, wlr_output->serial);
    }
    const edid_blob *edid = &output_display_of(record->output)->edid;
    int fd = edid->size > 0 ? edid_create_fd(edid) : -1;
    if (fd >= 0) {
        xfce_output_v1_send_edid(record->resource, fd, (uint32_t) edid->size);
        close(fd);
    }
}

static bool send_workarea(output_record *record)
{
    wm_rect workarea = relative_workarea(record->output);
    if (wm_rect_equals(&workarea, &record->workarea)) {
        return false;
    }
    record->workarea = workarea;
    xfce_output_v1_send_workarea(record->resource, workarea.x, workarea.y,
                                 (uint32_t) workarea.width, (uint32_t) workarea.height);
    return true;
}

static bool send_primary(output_record *record)
{
    bool is_primary = output_display_of(record->output)->is_primary;
    bool becomes_primary = is_primary && !record->sent_primary;
    record->sent_primary = is_primary;
    if (becomes_primary) {
        xfce_output_v1_send_primary(record->resource);
    }
    return becomes_primary;
}

static wm_rect relative_workarea(const output *o)
{
    wm_rect full = output_full(o);
    wm_rect usable = output_usable(o);
    return (wm_rect){usable.x - full.x, usable.y - full.y, usable.width, usable.height};
}

static void send_pointer(output_record *record, struct wlr_seat *seat, pointer_event event)
{
    struct wl_client *client = wl_resource_get_client(record->resource);
    struct wlr_seat_client *seat_client = wlr_seat_client_for_wl_client(seat, client);
    if (seat_client == NULL) {
        return;
    }
    struct wl_resource *seat_resource;
    wl_resource_for_each(seat_resource, &seat_client->resources) {
        event(record->resource, seat_resource);
    }
}
