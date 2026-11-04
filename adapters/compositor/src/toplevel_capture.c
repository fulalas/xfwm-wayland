#include <stdlib.h>
#include <wlr/types/wlr_ext_image_capture_source_v1.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/util/log.h>

#include "foreign_toplevel.h"
#include "listener.h"
#include "toplevel_capture.h"

typedef struct capture_entry {
    toplevel_capture *owner;
    view *view;
    struct wlr_scene *scene;
    struct wlr_ext_image_capture_source_v1 *source;
    struct wl_listener source_destroy;
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
static void handle_source_destroy(struct wl_listener *listener, void *data);
static capture_entry *entry_for(toplevel_capture *capture, view *view);
static capture_entry *entry_create(toplevel_capture *capture, view *view);
static bool entry_ensure_source(capture_entry *entry);
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
    if (entry == NULL || !entry_ensure_source(entry)) {
        return;
    }
    wlr_ext_foreign_toplevel_image_capture_source_manager_v1_request_accept(request,
                                                                            entry->source);
}

static void handle_unmapped(struct wl_listener *listener, void *data)
{
    toplevel_capture *c = wl_container_of(listener, c, unmapped);
    capture_entry *entry;
    wl_list_for_each(entry, &c->entries, link) {
        if (entry->view == data) {
            entry_destroy(entry);
            return;
        }
    }
}

static void handle_source_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    capture_entry *entry = wl_container_of(listener, entry, source_destroy);
    listener_disconnect(&entry->source_destroy);
    entry->source = NULL;
}

static capture_entry *entry_for(toplevel_capture *c, view *v)
{
    capture_entry *entry;
    wl_list_for_each(entry, &c->entries, link) {
        if (entry->view == v) {
            return entry;
        }
    }
    return entry_create(c, v);
}

static capture_entry *entry_create(toplevel_capture *c, view *v)
{
    capture_entry *entry = calloc(1, sizeof(*entry));
    if (entry == NULL) {
        return NULL;
    }
    entry->owner = c;
    entry->view = v;
    wl_list_init(&entry->source_destroy.link);
    entry->scene = wlr_scene_create();
    if (entry->scene == NULL ||
        wlr_scene_subsurface_tree_create(&entry->scene->tree, view_surface(v)) == NULL) {
        wlr_log(WLR_ERROR, "cannot build the capture scene of a window");
        if (entry->scene != NULL) {
            wlr_scene_node_destroy(&entry->scene->tree.node);
        }
        free(entry);
        return NULL;
    }
    wl_list_insert(&c->entries, &entry->link);
    return entry;
}

static bool entry_ensure_source(capture_entry *entry)
{
    if (entry->source != NULL) {
        return true;
    }
    compositor *server = entry->owner->server;
    entry->source = wlr_ext_image_capture_source_v1_create_with_scene_node(
        &entry->scene->tree.node, server->loop, server->allocator, server->renderer);
    if (entry->source == NULL) {
        wlr_log(WLR_ERROR, "cannot create the capture source of a window");
        return false;
    }
    listener_connect(&entry->source_destroy, &entry->source->events.destroy,
                     handle_source_destroy);
    return true;
}

static void entry_destroy(capture_entry *entry)
{
    listener_disconnect(&entry->source_destroy);
    wlr_scene_node_destroy(&entry->scene->tree.node);
    wl_list_remove(&entry->link);
    free(entry);
}
