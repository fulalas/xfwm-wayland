#include <limits.h>
#include <stdlib.h>
#include <wlr/types/wlr_output.h>
#include <wlr/util/log.h>

#include "clock.h"
#include "listener.h"
#include "timing_surface.h"

typedef struct timing_waiter {
    uint32_t seq;
    int64_t target_ns;
    struct wl_list link;
} timing_waiter;

static void timing_surface_destroy(timing_surface *ts);
static void handle_client_commit(struct wl_listener *listener, void *data);
static void queue_waiter(timing_surface *ts);
static void release_due(timing_surface *ts);
static bool target_due(const timing_surface *ts, int64_t target_ns);
static void rearm_timer(timing_surface *ts);
static int64_t next_check(const timing_surface *ts, int64_t target_ns, int64_t now_ns);
static int handle_timer(void *data);
static void addon_destroy(struct wlr_addon *addon);

static const struct wlr_addon_interface addon_impl = {
    .name = "xfwm-commit-timer",
    .destroy = addon_destroy,
};

timing_surface *timing_surface_get(present_sync *sync, struct wlr_surface *surface)
{
    struct wlr_addon *addon = wlr_addon_find(&surface->addons, sync, &addon_impl);
    if (addon == NULL) {
        return NULL;
    }
    timing_surface *ts = wl_container_of(addon, ts, addon);
    return ts;
}

timing_surface *timing_surface_create(present_sync *sync, struct wlr_surface *surface)
{
    timing_surface *ts = calloc(1, sizeof(*ts));
    if (ts == NULL) {
        return NULL;
    }
    ts->timer = wl_event_loop_add_timer(sync->loop, handle_timer, ts);
    if (ts->timer == NULL) {
        wlr_log(WLR_ERROR, "cannot create the commit timer of a surface");
        free(ts);
        return NULL;
    }
    ts->sync = sync;
    ts->surface = surface;
    wl_list_init(&ts->waiters);
    wl_list_insert(&sync->timing_surfaces, &ts->link);
    wlr_addon_init(&ts->addon, &surface->addons, sync, &addon_impl);
    listener_connect(&ts->client_commit, &surface->events.client_commit, handle_client_commit);
    return ts;
}

void commit_timing_output_frame(present_sync *sync, struct wlr_output *output)
{
    timing_surface *ts;
    timing_surface *tmp;
    wl_list_for_each_safe(ts, tmp, &sync->timing_surfaces, link) {
        if (!wl_list_empty(&ts->waiters) && present_surface_on_output(ts->surface, output)) {
            release_due(ts);
        }
    }
}

static void timing_surface_destroy(timing_surface *ts)
{
    timing_waiter *waiter;
    timing_waiter *tmp;
    wl_list_for_each_safe(waiter, tmp, &ts->waiters, link) {
        wl_list_remove(&waiter->link);
        free(waiter);
    }
    if (ts->resource != NULL) {
        wl_resource_set_user_data(ts->resource, NULL);
    }
    listener_disconnect(&ts->client_commit);
    wl_event_source_remove(ts->timer);
    wl_list_remove(&ts->link);
    wlr_addon_finish(&ts->addon);
    free(ts);
}

static void handle_client_commit(struct wl_listener *listener, void *data)
{
    (void) data;
    timing_surface *ts = wl_container_of(listener, ts, client_commit);
    if (!ts->has_target) {
        return;
    }
    ts->has_target = false;
    if (!target_due(ts, ts->target_ns)) {
        queue_waiter(ts);
    }
}

static void queue_waiter(timing_surface *ts)
{
    timing_waiter *waiter = calloc(1, sizeof(*waiter));
    if (waiter == NULL) {
        wlr_log(WLR_ERROR, "cannot hold a timed commit, applying it now");
        return;
    }
    waiter->seq = wlr_surface_lock_pending(ts->surface);
    waiter->target_ns = ts->target_ns;
    wl_list_insert(ts->waiters.prev, &waiter->link);
    rearm_timer(ts);
}

static void release_due(timing_surface *ts)
{
    timing_waiter *waiter;
    timing_waiter *tmp;
    wl_list_for_each_safe(waiter, tmp, &ts->waiters, link) {
        if (target_due(ts, waiter->target_ns)) {
            uint32_t seq = waiter->seq;
            wl_list_remove(&waiter->link);
            free(waiter);
            wlr_surface_unlock_cached(ts->surface, seq);
        }
    }
    rearm_timer(ts);
}

static bool target_due(const timing_surface *ts, int64_t target_ns)
{
    int64_t now = clock_now_ns();
    bool on_screen = false;
    struct wlr_surface_output *entry;
    wl_list_for_each(entry, &ts->surface->current_outputs, link) {
        const vblank_clock *clock = present_sync_clock(ts->sync, entry->output);
        if (clock == NULL) {
            continue;
        }
        on_screen = true;
        output_moment moment = {now, entry->output->frame_pending};
        if (target_ns <= timing_earliest_present(clock, moment)) {
            return true;
        }
    }
    return !on_screen && target_ns <= now;
}

static void rearm_timer(timing_surface *ts)
{
    int64_t now = clock_now_ns();
    int64_t soonest = INT64_MAX;
    timing_waiter *waiter;
    wl_list_for_each(waiter, &ts->waiters, link) {
        int64_t check = next_check(ts, waiter->target_ns, now);
        soonest = check < soonest ? check : soonest;
    }
    if (soonest == INT64_MAX) {
        wl_event_source_timer_update(ts->timer, 0);
        return;
    }
    int64_t delay_ms = soonest > now ? (soonest - now + CLOCK_NS_PER_MS - 1) / CLOCK_NS_PER_MS : 1;
    wl_event_source_timer_update(ts->timer, delay_ms < INT_MAX ? (int) delay_ms : INT_MAX);
}

static int64_t next_check(const timing_surface *ts, int64_t target_ns, int64_t now_ns)
{
    int64_t soonest = target_ns;
    struct wlr_surface_output *entry;
    wl_list_for_each(entry, &ts->surface->current_outputs, link) {
        const vblank_clock *clock = present_sync_clock(ts->sync, entry->output);
        if (clock == NULL) {
            continue;
        }
        int64_t check = timing_next_check(clock, target_ns, now_ns);
        soonest = check < soonest ? check : soonest;
    }
    return soonest;
}

static int handle_timer(void *data)
{
    timing_surface *ts = data;
    release_due(ts);
    return 0;
}

static void addon_destroy(struct wlr_addon *addon)
{
    timing_surface *ts = wl_container_of(addon, ts, addon);
    timing_surface_destroy(ts);
}
