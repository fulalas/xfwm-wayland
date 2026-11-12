#include "surface_watch.h"

#include <stdlib.h>
#include <wlr/types/wlr_compositor.h>

#include "seat_private.h"
#include "util/listener.h"

struct surface_watch {
    seat *seat;
    struct wl_list surfaces;
    struct wl_listener new_surface;
};

typedef struct watched_surface {
    surface_watch *watch;
    struct wlr_surface *surface;
    int width;
    int height;
    struct wl_list link;
    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener commit;
    struct wl_listener destroy;
} watched_surface;

static void handle_new_surface(struct wl_listener *listener, void *data);
static void handle_map(struct wl_listener *listener, void *data);
static void handle_unmap(struct wl_listener *listener, void *data);
static void handle_commit(struct wl_listener *listener, void *data);
static void handle_destroy(struct wl_listener *listener, void *data);
static bool reshaped(watched_surface *watched);
static void forget(watched_surface *watched);

surface_watch *surface_watch_create(seat *s)
{
    surface_watch *w = calloc(1, sizeof(*w));
    if (w == NULL) {
        return NULL;
    }
    w->seat = s;
    wl_list_init(&w->surfaces);
    listener_connect(&w->new_surface, &s->server->globals.compositor->events.new_surface,
                     handle_new_surface);
    return w;
}

void surface_watch_destroy(surface_watch *w)
{
    watched_surface *watched;
    watched_surface *tmp;
    wl_list_for_each_safe(watched, tmp, &w->surfaces, link) {
        forget(watched);
    }
    listener_disconnect(&w->new_surface);
    free(w);
}

static void handle_new_surface(struct wl_listener *listener, void *data)
{
    surface_watch *w = wl_container_of(listener, w, new_surface);
    struct wlr_surface *surface = data;
    watched_surface *watched = calloc(1, sizeof(*watched));
    if (watched == NULL) {
        return;
    }
    watched->watch = w;
    watched->surface = surface;
    listener_connect(&watched->map, &surface->events.map, handle_map);
    listener_connect(&watched->unmap, &surface->events.unmap, handle_unmap);
    listener_connect(&watched->commit, &surface->events.commit, handle_commit);
    listener_connect(&watched->destroy, &surface->events.destroy, handle_destroy);
    wl_list_insert(&w->surfaces, &watched->link);
}

static void handle_map(struct wl_listener *listener, void *data)
{
    (void) data;
    watched_surface *watched = wl_container_of(listener, watched, map);
    seat_repick_pointer(watched->watch->seat);
}

static void handle_unmap(struct wl_listener *listener, void *data)
{
    (void) data;
    watched_surface *watched = wl_container_of(listener, watched, unmap);
    seat_repick_pointer(watched->watch->seat);
}

static void handle_commit(struct wl_listener *listener, void *data)
{
    (void) data;
    watched_surface *watched = wl_container_of(listener, watched, commit);
    if (reshaped(watched) && watched->surface->mapped) {
        seat_repick_pointer(watched->watch->seat);
    }
}

static void handle_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    watched_surface *watched = wl_container_of(listener, watched, destroy);
    forget(watched);
}

static bool reshaped(watched_surface *watched)
{
    const struct wlr_surface_state *now = &watched->surface->current;
    bool resized = now->width != watched->width || now->height != watched->height;
    watched->width = now->width;
    watched->height = now->height;
    return resized || (now->committed & WLR_SURFACE_STATE_INPUT_REGION) != 0;
}

static void forget(watched_surface *watched)
{
    listener_disconnect(&watched->map);
    listener_disconnect(&watched->unmap);
    listener_disconnect(&watched->commit);
    listener_disconnect(&watched->destroy);
    wl_list_remove(&watched->link);
    free(watched);
}
