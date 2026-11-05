#include <stdlib.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_session_lock_v1.h>
#include <wlr/util/log.h>

#include "listener.h"
#include "lock.h"
#include "node_owner.h"
#include "output.h"
#include "scene_layers.h"
#include "seat.h"

static const float BACKGROUND_COLOR[4] = {0.0f, 0.0f, 0.0f, 1.0f};

typedef struct lock_surface {
    session_lock *lock;
    struct wlr_session_lock_surface_v1 *wlr;
    struct wlr_scene_tree *tree;
    node_owner owner;
    struct wl_list link;
    struct wl_listener map;
    struct wl_listener destroy;
} lock_surface;

struct session_lock {
    compositor *server;
    struct wlr_session_lock_manager_v1 *manager;
    struct wlr_session_lock_v1 *active;
    bool locked;
    struct wlr_scene_tree *tree;
    struct wlr_scene_tree *backgrounds;
    struct wl_list surfaces;
    struct wl_listener new_lock;
    struct wl_listener new_surface;
    struct wl_listener unlock;
    struct wl_listener lock_destroy;
};

static void handle_new_lock(struct wl_listener *listener, void *data);
static void handle_new_surface(struct wl_listener *listener, void *data);
static void handle_unlock(struct wl_listener *listener, void *data);
static void handle_lock_destroy(struct wl_listener *listener, void *data);
static void handle_surface_map(struct wl_listener *listener, void *data);
static void handle_surface_destroy(struct wl_listener *listener, void *data);
static void forget_active(session_lock *lock);
static void rebuild_backgrounds(session_lock *lock);
static void place_surface(lock_surface *surface);
static void lock_changed(session_lock *lock);

session_lock *session_lock_create(compositor *server)
{
    session_lock *lock = calloc(1, sizeof(*lock));
    if (lock == NULL) {
        return NULL;
    }
    lock->server = server;
    wl_list_init(&lock->surfaces);
    wl_list_init(&lock->new_surface.link);
    wl_list_init(&lock->unlock.link);
    wl_list_init(&lock->lock_destroy.link);
    lock->manager = wlr_session_lock_manager_v1_create(server->display);
    lock->tree = wlr_scene_tree_create(scene_layers_tree(server->layers, SCENE_LAYER_LOCK));
    lock->backgrounds = lock->tree != NULL ? wlr_scene_tree_create(lock->tree) : NULL;
    if (lock->manager == NULL || lock->backgrounds == NULL) {
        wlr_log(WLR_ERROR, "cannot create the session lock");
        free(lock);
        return NULL;
    }
    wlr_scene_node_set_enabled(&lock->tree->node, false);
    listener_connect(&lock->new_lock, &lock->manager->events.new_lock, handle_new_lock);
    return lock;
}

void session_lock_destroy(session_lock *lock)
{
    forget_active(lock);
    listener_disconnect(&lock->new_lock);
    free(lock);
}

bool session_lock_is_locked(const session_lock *lock)
{
    return lock->locked;
}

struct wlr_surface *session_lock_focus(const session_lock *lock)
{
    wm_point pointer = seat_pointer_position(lock->server->seat);
    output *under = outputs_at_point(lock->server->outputs, pointer);
    struct wlr_surface *fallback = NULL;
    lock_surface *surface;
    wl_list_for_each(surface, &lock->surfaces, link) {
        if (!surface->wlr->surface->mapped) {
            continue;
        }
        if (under != NULL && surface->wlr->output == output_wlr(under)) {
            return surface->wlr->surface;
        }
        fallback = fallback != NULL ? fallback : surface->wlr->surface;
    }
    return fallback;
}

void session_lock_arrange(session_lock *lock)
{
    rebuild_backgrounds(lock);
    lock_surface *surface;
    wl_list_for_each(surface, &lock->surfaces, link) {
        place_surface(surface);
    }
}

static void handle_new_lock(struct wl_listener *listener, void *data)
{
    session_lock *lock = wl_container_of(listener, lock, new_lock);
    struct wlr_session_lock_v1 *wlr_lock = data;
    if (lock->active != NULL) {
        wlr_session_lock_v1_destroy(wlr_lock);
        return;
    }
    lock->active = wlr_lock;
    lock->locked = true;
    listener_connect(&lock->new_surface, &wlr_lock->events.new_surface, handle_new_surface);
    listener_connect(&lock->unlock, &wlr_lock->events.unlock, handle_unlock);
    listener_connect(&lock->lock_destroy, &wlr_lock->events.destroy, handle_lock_destroy);
    session_lock_arrange(lock);
    wlr_session_lock_v1_send_locked(wlr_lock);
    lock_changed(lock);
}

static void handle_new_surface(struct wl_listener *listener, void *data)
{
    session_lock *lock = wl_container_of(listener, lock, new_surface);
    struct wlr_session_lock_surface_v1 *wlr = data;
    lock_surface *surface = calloc(1, sizeof(*surface));
    if (surface == NULL) {
        return;
    }
    surface->lock = lock;
    surface->wlr = wlr;
    surface->tree = wlr_scene_subsurface_tree_create(lock->tree, wlr->surface);
    surface->owner = (node_owner){NODE_LOCK_SURFACE, surface};
    if (surface->tree != NULL) {
        node_owner_attach(&surface->tree->node, &surface->owner);
    }
    listener_connect(&surface->map, &wlr->surface->events.map, handle_surface_map);
    listener_connect(&surface->destroy, &wlr->events.destroy, handle_surface_destroy);
    wl_list_insert(&lock->surfaces, &surface->link);
    place_surface(surface);
}

static void handle_unlock(struct wl_listener *listener, void *data)
{
    (void) data;
    session_lock *lock = wl_container_of(listener, lock, unlock);
    forget_active(lock);
    lock->locked = false;
    lock_changed(lock);
}

static void handle_lock_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    session_lock *lock = wl_container_of(listener, lock, lock_destroy);
    forget_active(lock);
    lock_changed(lock);
}

static void handle_surface_map(struct wl_listener *listener, void *data)
{
    (void) data;
    lock_surface *surface = wl_container_of(listener, surface, map);
    seat_focus_refresh(surface->lock->server->seat);
}

static void handle_surface_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    lock_surface *surface = wl_container_of(listener, surface, destroy);
    session_lock *lock = surface->lock;
    struct wlr_surface *wlr_surface = surface->wlr->surface;
    listener_disconnect(&surface->map);
    listener_disconnect(&surface->destroy);
    wl_list_remove(&surface->link);
    if (surface->tree != NULL) {
        wlr_scene_node_destroy(&surface->tree->node);
    }
    free(surface);
    seat_surface_gone(lock->server->seat, wlr_surface);
}

static void forget_active(session_lock *lock)
{
    listener_disconnect(&lock->new_surface);
    listener_disconnect(&lock->unlock);
    listener_disconnect(&lock->lock_destroy);
    lock->active = NULL;
}

static void rebuild_backgrounds(session_lock *lock)
{
    struct wlr_scene_node *child;
    struct wlr_scene_node *tmp;
    wl_list_for_each_safe(child, tmp, &lock->backgrounds->children, link) {
        wlr_scene_node_destroy(child);
    }
    outputs *outs = lock->server->outputs;
    for (output *o = outputs_next_active(outs, NULL); o != NULL; o = outputs_next_active(outs, o)) {
        wm_rect full = output_full(o);
        struct wlr_scene_rect *rect =
            wlr_scene_rect_create(lock->backgrounds, full.width, full.height, BACKGROUND_COLOR);
        if (rect != NULL) {
            wlr_scene_node_set_position(&rect->node, full.x, full.y);
        }
    }
}

static void place_surface(lock_surface *surface)
{
    output *o = output_from_wlr(surface->wlr->output);
    if (o == NULL || surface->tree == NULL) {
        return;
    }
    wm_rect full = output_full(o);
    wlr_scene_node_set_position(&surface->tree->node, full.x, full.y);
    wlr_session_lock_surface_v1_configure(surface->wlr, (uint32_t) full.width,
                                          (uint32_t) full.height);
}

static void lock_changed(session_lock *lock)
{
    wlr_scene_node_set_enabled(&lock->tree->node, lock->locked);
    seat_release_pointer_grab(lock->server->seat);
    seat_focus_refresh(lock->server->seat);
}
