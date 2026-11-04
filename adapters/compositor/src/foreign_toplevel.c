#include <stdlib.h>
#include <wlr/types/wlr_ext_foreign_toplevel_list_v1.h>
#include <wlr/types/wlr_output_layout.h>

#include "foreign_toplevel_private.h"
#include "listener.h"

static void handle_mapped(struct wl_listener *listener, void *data);
static void handle_changed(struct wl_listener *listener, void *data);
static void handle_unmapped(struct wl_listener *listener, void *data);
static void handle_layout_change(struct wl_listener *listener, void *data);
static void sync_children(foreign_toplevels *toplevels, const view *parent);
static void entry_destroy(toplevel_entry *entry);

foreign_toplevels *foreign_toplevels_create(compositor *server)
{
    foreign_toplevels *t = calloc(1, sizeof(*t));
    if (t == NULL) {
        return NULL;
    }
    t->server = server;
    wl_list_init(&t->entries);
    view_signals *events = &server->view_events;
    listener_connect(&t->mapped, &events->mapped, handle_mapped);
    listener_connect(&t->changed, &events->changed, handle_changed);
    listener_connect(&t->unmapped, &events->unmapped, handle_unmapped);
    listener_connect(&t->layout_change, &server->layout->events.change, handle_layout_change);
    return t;
}

void foreign_toplevels_destroy(foreign_toplevels *t)
{
    toplevel_entry *entry;
    toplevel_entry *tmp;
    wl_list_for_each_safe(entry, tmp, &t->entries, link) {
        entry_destroy(entry);
    }
    listener_disconnect(&t->mapped);
    listener_disconnect(&t->changed);
    listener_disconnect(&t->unmapped);
    listener_disconnect(&t->layout_change);
    free(t);
}

view *foreign_toplevel_ext_view(const struct wlr_ext_foreign_toplevel_handle_v1 *handle)
{
    const toplevel_entry *entry = handle != NULL ? handle->data : NULL;
    return entry != NULL ? entry->view : NULL;
}

toplevel_entry *foreign_toplevels_find(const foreign_toplevels *t, const view *v)
{
    toplevel_entry *entry;
    wl_list_for_each(entry, &t->entries, link) {
        if (entry->view == v) {
            return entry;
        }
    }
    return NULL;
}

static void handle_mapped(struct wl_listener *listener, void *data)
{
    foreign_toplevels *t = wl_container_of(listener, t, mapped);
    toplevel_entry *entry = calloc(1, sizeof(*entry));
    if (entry == NULL) {
        return;
    }
    entry->owner = t;
    entry->view = data;
    wl_list_insert(t->entries.prev, &entry->link);
    toplevel_entry_sync(entry);
    sync_children(t, entry->view);
}

static void handle_changed(struct wl_listener *listener, void *data)
{
    foreign_toplevels *t = wl_container_of(listener, t, changed);
    toplevel_entry *entry = foreign_toplevels_find(t, data);
    if (entry != NULL) {
        toplevel_entry_sync(entry);
    }
}

static void handle_unmapped(struct wl_listener *listener, void *data)
{
    foreign_toplevels *t = wl_container_of(listener, t, unmapped);
    toplevel_entry *entry = foreign_toplevels_find(t, data);
    if (entry != NULL) {
        entry_destroy(entry);
    }
}

static void handle_layout_change(struct wl_listener *listener, void *data)
{
    (void) data;
    foreign_toplevels *t = wl_container_of(listener, t, layout_change);
    toplevel_entry *entry;
    wl_list_for_each(entry, &t->entries, link) {
        toplevel_entry_sync(entry);
    }
}

static void sync_children(foreign_toplevels *t, const view *parent)
{
    toplevel_entry *entry;
    wl_list_for_each(entry, &t->entries, link) {
        if (view_state(entry->view)->parent == parent) {
            toplevel_entry_sync(entry);
        }
    }
}

static void entry_destroy(toplevel_entry *entry)
{
    toplevel_entry_hide(entry);
    wl_list_remove(&entry->link);
    free(entry);
}
