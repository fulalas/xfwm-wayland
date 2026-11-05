#include "output_head.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wlr/types/wlr_output.h>

#include "listener.h"
#include "output_setting.h"
#include "resource.h"
#include "xfce-output-management-private-v1-protocol.h"

static void handle_output_destroy(struct wl_listener *listener, void *data);
static bool copy_edid(edid_blob *copy, const edid_blob *source);
static void send_edid(const output_head *head, struct wl_resource *resource);

static const struct xfce_output_head_private_v1_interface head_implementation = {
    .release = resource_handle_destroy,
};

output_head *output_head_create(output *o, struct wl_list *heads)
{
    output_head *head = calloc(1, sizeof(*head));
    if (head == NULL) {
        return NULL;
    }
    head->wlr_output = output_wlr(o);
    head->name = strdup(head->wlr_output->name);
    if (head->name == NULL || !copy_edid(&head->edid, &output_display_of(o)->edid)) {
        free(head->name);
        free(head);
        return NULL;
    }
    wl_list_init(&head->resources);
    listener_connect(&head->output_destroy, &head->wlr_output->events.destroy,
                     handle_output_destroy);
    wl_list_insert(heads->prev, &head->link);
    return head;
}

void output_head_announce(output_head *head, struct wl_resource *manager)
{
    struct wl_client *client = wl_resource_get_client(manager);
    struct wl_resource *resource = wl_resource_create(
        client, &xfce_output_head_private_v1_interface, wl_resource_get_version(manager), 0);
    if (resource == NULL) {
        wl_client_post_no_memory(client);
        return;
    }
    wl_resource_set_implementation(resource, &head_implementation, head, resource_unlink);
    wl_list_insert(&head->resources, wl_resource_get_link(resource));
    xfce_output_management_manager_private_v1_send_head(manager, resource, head->name);
    send_edid(head, resource);
}

void output_head_destroy(output_head *head)
{
    struct wl_resource *resource;
    struct wl_resource *next;
    wl_resource_for_each_safe(resource, next, &head->resources) {
        xfce_output_head_private_v1_send_finished(resource);
        resource_detach(resource);
    }
    listener_disconnect(&head->output_destroy);
    wl_list_remove(&head->link);
    edid_release(&head->edid);
    free(head->name);
    free(head);
}

static void handle_output_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    output_head *head = wl_container_of(listener, head, output_destroy);
    listener_disconnect(&head->output_destroy);
    head->wlr_output = NULL;
}

static bool copy_edid(edid_blob *copy, const edid_blob *source)
{
    *copy = (edid_blob){0};
    if (source->size == 0) {
        return true;
    }
    copy->data = malloc(source->size);
    if (copy->data == NULL) {
        return false;
    }
    memcpy(copy->data, source->data, source->size);
    copy->size = source->size;
    return true;
}

static void send_edid(const output_head *head, struct wl_resource *resource)
{
    int fd = head->edid.size > 0 ? edid_create_fd(&head->edid) : -1;
    if (fd >= 0) {
        xfce_output_head_private_v1_send_edid(resource, fd, (uint32_t) head->edid.size);
        close(fd);
    }
}
