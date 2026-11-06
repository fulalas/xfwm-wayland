#include <stdlib.h>
#include <wlr/types/wlr_ext_image_capture_source_v1.h>

#include "foreign_toplevel.h"
#include "listener.h"
#include "toplevel_capture.h"
#include "window_capture_source.h"

typedef struct capture_entry {
    window_capture_source *source;
    struct wl_list link;
} capture_entry;

struct toplevel_capture {
    compositor *server;
    struct wl_list entries;
    struct wl_listener new_request;
    struct wl_listener unmapped;
};

static void handle_new_request(struct wl_listener *listener, void *data);
static void handle_unmapped(struct wl_listener *listener, void *data);
static capture_entry *entry_for(toplevel_capture *capture, view *view);
static void entry_destroy(capture_entry *entry);

toplevel_capture *toplevel_capture_create(compositor *server)
{
    toplevel_capture *c = calloc(1, sizeof(*c));
    if (c == NULL) {
        return NULL;
    }
    c->server = server;
    wl_list_init(&c->entries);
    wl_list_init(&c->new_request.link);
    listener_connect(&c->unmapped, &server->view_events.unmapped, handle_unmapped);
    if (server->globals.toplevel_capture != NULL) {
        listener_connect(&c->new_request, &server->globals.toplevel_capture->events.new_request,
                         handle_new_request);
    }
    return c;
}

void toplevel_capture_destroy(toplevel_capture *c)
{
    toplevel_capture_drop_sources(c);
    listener_disconnect(&c->new_request);
    listener_disconnect(&c->unmapped);
    free(c);
}

void toplevel_capture_drop_sources(toplevel_capture *c)
{
    capture_entry *entry;
    capture_entry *tmp;
    wl_list_for_each_safe(entry, tmp, &c->entries, link) {
        entry_destroy(entry);
    }
}

static void handle_new_request(struct wl_listener *listener, void *data)
{
    toplevel_capture *c = wl_container_of(listener, c, new_request);
    struct wlr_ext_foreign_toplevel_image_capture_source_manager_v1_request *request = data;
    view *v = foreign_toplevel_ext_view(request->toplevel_handle);
    capture_entry *entry = v != NULL ? entry_for(c, v) : NULL;
    if (entry == NULL) {
        return;
    }
    wlr_ext_foreign_toplevel_image_capture_source_manager_v1_request_accept(
        request, window_capture_source_base(entry->source));
}

static void handle_unmapped(struct wl_listener *listener, void *data)
{
    toplevel_capture *c = wl_container_of(listener, c, unmapped);
    capture_entry *entry;
    wl_list_for_each(entry, &c->entries, link) {
        if (window_capture_source_view(entry->source) == data) {
            entry_destroy(entry);
            return;
        }
    }
}

static capture_entry *entry_for(toplevel_capture *c, view *v)
{
    capture_entry *entry;
    wl_list_for_each(entry, &c->entries, link) {
        if (window_capture_source_view(entry->source) == v) {
            return entry;
        }
    }
    entry = calloc(1, sizeof(*entry));
    if (entry == NULL) {
        return NULL;
    }
    entry->source = window_capture_source_create(c->server, v);
    if (entry->source == NULL) {
        free(entry);
        return NULL;
    }
    wl_list_insert(&c->entries, &entry->link);
    return entry;
}

static void entry_destroy(capture_entry *entry)
{
    window_capture_source_destroy(entry->source);
    wl_list_remove(&entry->link);
    free(entry);
}
