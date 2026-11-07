#include <stdlib.h>
#include <wlr/backend.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_output.h>
#include <wlr/util/log.h>

#include "util/clock.h"
#include "util/listener.h"
#include "present_sync_private.h"

static void handle_new_output(struct wl_listener *listener, void *data);
static void handle_commit(struct wl_listener *listener, void *data);
static void handle_present(struct wl_listener *listener, void *data);
static void handle_frame(struct wl_listener *listener, void *data);
static void handle_output_destroy(struct wl_listener *listener, void *data);
static void output_clock_destroy(output_clock *clock);
static bool clock_is_predictable(const struct wlr_output_event_present *event);

present_sync *present_sync_create(compositor *server)
{
    present_sync *sync = calloc(1, sizeof(*sync));
    if (sync == NULL) {
        return NULL;
    }
    sync->server = server;
    sync->loop = server->loop;
    wl_list_init(&sync->outputs);
    wl_list_init(&sync->fifo_surfaces);
    wl_list_init(&sync->timing_surfaces);
    listener_connect(&sync->new_output, &server->backend->events.new_output, handle_new_output);
    if (!fifo_global_create(sync) || !commit_timing_global_create(sync)) {
        wlr_log(WLR_ERROR, "cannot create the fifo and commit timing globals");
        present_sync_destroy(sync);
        return NULL;
    }
    return sync;
}

void present_sync_destroy(present_sync *sync)
{
    output_clock *clock;
    output_clock *tmp;
    wl_list_for_each_safe(clock, tmp, &sync->outputs, link) {
        output_clock_destroy(clock);
    }
    listener_disconnect(&sync->new_output);
    if (sync->fifo_idle != NULL) {
        wl_event_source_remove(sync->fifo_idle);
    }
    if (sync->fifo_global != NULL) {
        wl_global_destroy(sync->fifo_global);
    }
    if (sync->timing_global != NULL) {
        wl_global_destroy(sync->timing_global);
    }
    free(sync);
}

const vblank_clock *present_sync_clock(const present_sync *sync, const struct wlr_output *output)
{
    output_clock *clock;
    wl_list_for_each(clock, &sync->outputs, link) {
        if (clock->output == output) {
            return &clock->clock;
        }
    }
    return NULL;
}

bool present_surface_on_output(struct wlr_surface *surface, const struct wlr_output *output)
{
    struct wlr_surface_output *entry;
    wl_list_for_each(entry, &surface->current_outputs, link) {
        if (entry->output == output) {
            return true;
        }
    }
    return false;
}

bool present_surface_on_screen(const present_sync *sync, struct wlr_surface *surface)
{
    struct wlr_surface_output *entry;
    wl_list_for_each(entry, &surface->current_outputs, link) {
        if (present_sync_clock(sync, entry->output) != NULL) {
            return true;
        }
    }
    return false;
}

void present_surface_schedule_frames(struct wlr_surface *surface)
{
    struct wlr_surface_output *entry;
    wl_list_for_each(entry, &surface->current_outputs, link) {
        wlr_output_schedule_frame(entry->output);
    }
}

static void handle_new_output(struct wl_listener *listener, void *data)
{
    present_sync *sync = wl_container_of(listener, sync, new_output);
    struct wlr_output *output = data;
    if (output->non_desktop) {
        return;
    }
    output_clock *clock = calloc(1, sizeof(*clock));
    if (clock == NULL) {
        wlr_log(WLR_ERROR, "cannot track presentation timing of output %s", output->name);
        return;
    }
    clock->sync = sync;
    clock->output = output;
    listener_connect(&clock->commit, &output->events.commit, handle_commit);
    listener_connect(&clock->present, &output->events.present, handle_present);
    listener_connect(&clock->frame, &output->events.frame, handle_frame);
    listener_connect(&clock->destroy, &output->events.destroy, handle_output_destroy);
    wl_list_insert(&sync->outputs, &clock->link);
}

static void handle_commit(struct wl_listener *listener, void *data)
{
    (void) data;
    output_clock *clock = wl_container_of(listener, clock, commit);
    fifo_output_latched(clock->sync, clock->output);
}

static void handle_present(struct wl_listener *listener, void *data)
{
    output_clock *clock = wl_container_of(listener, clock, present);
    const struct wlr_output_event_present *event = data;
    if (!event->presented) {
        return;
    }
    clock->clock = (vblank_clock) {
        .last_ns = clock_timespec_ns(&event->when),
        .refresh_ns = event->refresh,
        .predictable = clock_is_predictable(event),
    };
}

static void handle_frame(struct wl_listener *listener, void *data)
{
    (void) data;
    output_clock *clock = wl_container_of(listener, clock, frame);
    commit_timing_output_frame(clock->sync, clock->output);
}

static void handle_output_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    output_clock *clock = wl_container_of(listener, clock, destroy);
    output_clock_destroy(clock);
}

static void output_clock_destroy(output_clock *clock)
{
    listener_disconnect(&clock->commit);
    listener_disconnect(&clock->present);
    listener_disconnect(&clock->frame);
    listener_disconnect(&clock->destroy);
    wl_list_remove(&clock->link);
    free(clock);
}

static bool clock_is_predictable(const struct wlr_output_event_present *event)
{
    const struct wlr_output *output = event->output;
    return (event->flags & WLR_OUTPUT_PRESENT_VSYNC) != 0 && event->refresh > 0 &&
           output->adaptive_sync_status != WLR_OUTPUT_ADAPTIVE_SYNC_ENABLED;
}
