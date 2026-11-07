#include <stdlib.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/util/log.h>

#include "fifo_surface.h"
#include "util/listener.h"

enum {
    WATCHDOG_MS = 1000,
};

typedef struct fifo_waiter {
    uint32_t seq;
    uint32_t needed;
    struct wl_list link;
} fifo_waiter;

static bool attach_state(fifo_surface *fs, struct wlr_surface *surface);
static void fifo_surface_destroy(fifo_surface *fs);
static void handle_client_commit(struct wl_listener *listener, void *data);
static bool is_synchronized(struct wlr_surface *surface);
static void queue_waiter(fifo_surface *fs);
static void release_ready(fifo_surface *fs);
static void update_watchdog(fifo_surface *fs);
static int handle_watchdog(void *data);
static void handle_release_idle(void *data);
static void synced_move_state(void *dst, void *src);
static void synced_commit(struct wlr_surface_synced *synced);
static void addon_destroy(struct wlr_addon *addon);
static void fifo_schedule_release(present_sync *sync);

static const struct wlr_surface_synced_impl synced_impl = {
    .state_size = sizeof(fifo_state),
    .move_state = synced_move_state,
    .commit = synced_commit,
};

static const struct wlr_addon_interface addon_impl = {
    .name = "xfwm-fifo-surface",
    .destroy = addon_destroy,
};

fifo_surface *fifo_surface_get(present_sync *sync, struct wlr_surface *surface)
{
    struct wlr_addon *addon = wlr_addon_find(&surface->addons, sync, &addon_impl);
    if (addon == NULL) {
        return NULL;
    }
    fifo_surface *fs = wl_container_of(addon, fs, addon);
    return fs;
}

fifo_surface *fifo_surface_create(present_sync *sync, struct wlr_surface *surface)
{
    fifo_surface *fs = calloc(1, sizeof(*fs));
    if (fs == NULL) {
        return NULL;
    }
    fs->sync = sync;
    if (!attach_state(fs, surface)) {
        wlr_log(WLR_ERROR, "cannot track the fifo state of a surface");
        free(fs);
        return NULL;
    }
    fs->surface = surface;
    wl_list_init(&fs->waiters);
    wl_list_insert(&sync->fifo_surfaces, &fs->link);
    wlr_addon_init(&fs->addon, &surface->addons, sync, &addon_impl);
    listener_connect(&fs->client_commit, &surface->events.client_commit, handle_client_commit);
    return fs;
}

void fifo_output_latched(present_sync *sync, struct wlr_output *output)
{
    fifo_surface *fs;
    wl_list_for_each(fs, &sync->fifo_surfaces, link) {
        if (fs->counters.barrier && present_surface_on_output(fs->surface, output)) {
            fs->counters.barrier = false;
            fifo_schedule_release(sync);
        }
    }
}

static void fifo_schedule_release(present_sync *sync)
{
    if (sync->fifo_idle == NULL) {
        sync->fifo_idle = wl_event_loop_add_idle(sync->loop, handle_release_idle, sync);
    }
}

static bool attach_state(fifo_surface *fs, struct wlr_surface *surface)
{
    fs->watchdog = wl_event_loop_add_timer(fs->sync->loop, handle_watchdog, fs);
    if (fs->watchdog == NULL) {
        return false;
    }
    if (wlr_surface_synced_init(&fs->synced, surface, &synced_impl, &fs->pending, &fs->current)) {
        return true;
    }
    wl_event_source_remove(fs->watchdog);
    return false;
}

static void fifo_surface_destroy(fifo_surface *fs)
{
    fifo_waiter *waiter;
    fifo_waiter *tmp;
    wl_list_for_each_safe(waiter, tmp, &fs->waiters, link) {
        wl_list_remove(&waiter->link);
        free(waiter);
    }
    if (fs->resource != NULL) {
        wl_resource_set_user_data(fs->resource, NULL);
    }
    listener_disconnect(&fs->client_commit);
    wl_event_source_remove(fs->watchdog);
    wl_list_remove(&fs->link);
    wlr_addon_finish(&fs->addon);
    wlr_surface_synced_finish(&fs->synced);
    free(fs);
}

static void handle_client_commit(struct wl_listener *listener, void *data)
{
    (void) data;
    fifo_surface *fs = wl_container_of(listener, fs, client_commit);
    bool wait = fs->wait_barrier;
    fs->wait_barrier = false;
    if (!present_surface_on_screen(fs->sync, fs->surface)) {
        fs->counters.barrier = false;
    } else if (wait && !is_synchronized(fs->surface) && fifo_must_wait(&fs->counters)) {
        queue_waiter(fs);
    }
    if (fs->pending.set_barrier) {
        fs->counters.committed++;
    }
}

static bool is_synchronized(struct wlr_surface *surface)
{
    struct wlr_subsurface *subsurface = wlr_subsurface_try_from_wlr_surface(surface);
    while (subsurface != NULL) {
        if (subsurface->synchronized) {
            return true;
        }
        subsurface = subsurface->parent != NULL
                         ? wlr_subsurface_try_from_wlr_surface(subsurface->parent)
                         : NULL;
    }
    return false;
}

static void queue_waiter(fifo_surface *fs)
{
    fifo_waiter *waiter = calloc(1, sizeof(*waiter));
    if (waiter == NULL) {
        wlr_log(WLR_ERROR, "cannot hold a fifo commit, applying it now");
        return;
    }
    waiter->seq = wlr_surface_lock_pending(fs->surface);
    waiter->needed = fs->counters.committed;
    wl_list_insert(fs->waiters.prev, &waiter->link);
    update_watchdog(fs);
    present_surface_schedule_frames(fs->surface);
}

static void release_ready(fifo_surface *fs)
{
    while (!wl_list_empty(&fs->waiters)) {
        fifo_waiter *waiter = wl_container_of(fs->waiters.next, waiter, link);
        if (!fifo_waiter_ready(&fs->counters, waiter->needed)) {
            break;
        }
        uint32_t seq = waiter->seq;
        wl_list_remove(&waiter->link);
        free(waiter);
        wlr_surface_unlock_cached(fs->surface, seq);
    }
    update_watchdog(fs);
}

static void update_watchdog(fifo_surface *fs)
{
    wl_event_source_timer_update(fs->watchdog, wl_list_empty(&fs->waiters) ? 0 : WATCHDOG_MS);
}

static int handle_watchdog(void *data)
{
    fifo_surface *fs = data;
    fs->counters.barrier = false;
    release_ready(fs);
    return 0;
}

static void handle_release_idle(void *data)
{
    present_sync *sync = data;
    sync->fifo_idle = NULL;
    fifo_surface *fs;
    wl_list_for_each(fs, &sync->fifo_surfaces, link) {
        if (!wl_list_empty(&fs->waiters)) {
            release_ready(fs);
        }
    }
}

static void synced_move_state(void *dst, void *src)
{
    fifo_state *target = dst;
    fifo_state *source = src;
    *target = *source;
    *source = (fifo_state) {0};
}

static void synced_commit(struct wlr_surface_synced *synced)
{
    fifo_surface *fs = wl_container_of(synced, fs, synced);
    if (fs->current.set_barrier) {
        fs->counters.barrier = true;
        fs->counters.applied++;
    }
}

static void addon_destroy(struct wlr_addon *addon)
{
    fifo_surface *fs = wl_container_of(addon, fs, addon);
    fifo_surface_destroy(fs);
}
