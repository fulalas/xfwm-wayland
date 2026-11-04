#include "commit-timing-v1-protocol.h"
#include "timing_surface.h"
#include "resource.h"

enum {
    COMMIT_TIMING_VERSION = 1,
};

static const int64_t NSEC_PER_SEC = 1000000000;

static void bind_manager(struct wl_client *client, void *data, uint32_t version, uint32_t id);
static void handle_get_timer(struct wl_client *client, struct wl_resource *resource, uint32_t id,
                             struct wl_resource *surface_resource);
static void handle_set_timestamp(struct wl_client *client, struct wl_resource *resource,
                                 uint32_t tv_sec_hi, uint32_t tv_sec_lo, uint32_t tv_nsec);
static int64_t timestamp_ns(uint32_t tv_sec_hi, uint32_t tv_sec_lo, uint32_t tv_nsec);
static void handle_resource_destroy(struct wl_resource *resource);

static const struct wp_commit_timing_manager_v1_interface manager_impl = {
    .destroy = resource_handle_destroy,
    .get_timer = handle_get_timer,
};

static const struct wp_commit_timer_v1_interface timer_impl = {
    .set_timestamp = handle_set_timestamp,
    .destroy = resource_handle_destroy,
};

bool commit_timing_global_create(present_sync *sync)
{
    sync->timing_global =
        wl_global_create(sync->server->display, &wp_commit_timing_manager_v1_interface,
                         COMMIT_TIMING_VERSION, sync, bind_manager);
    return sync->timing_global != NULL;
}

static void bind_manager(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    struct wl_resource *resource =
        wl_resource_create(client, &wp_commit_timing_manager_v1_interface, (int) version, id);
    if (resource == NULL) {
        wl_client_post_no_memory(client);
        return;
    }
    wl_resource_set_implementation(resource, &manager_impl, data, NULL);
}

static void handle_get_timer(struct wl_client *client, struct wl_resource *resource, uint32_t id,
                             struct wl_resource *surface_resource)
{
    present_sync *sync = wl_resource_get_user_data(resource);
    struct wlr_surface *surface = wlr_surface_from_resource(surface_resource);
    timing_surface *ts = timing_surface_get(sync, surface);
    if (ts != NULL && ts->resource != NULL) {
        wl_resource_post_error(resource, WP_COMMIT_TIMING_MANAGER_V1_ERROR_COMMIT_TIMER_EXISTS,
                               "a commit timer already exists for this surface");
        return;
    }
    if (ts == NULL) {
        ts = timing_surface_create(sync, surface);
    }
    int version = wl_resource_get_version(resource);
    struct wl_resource *timer =
        ts != NULL ? wl_resource_create(client, &wp_commit_timer_v1_interface, version, id) : NULL;
    if (timer == NULL) {
        wl_resource_post_no_memory(resource);
        return;
    }
    wl_resource_set_implementation(timer, &timer_impl, ts, handle_resource_destroy);
    ts->resource = timer;
}

static void handle_set_timestamp(struct wl_client *client, struct wl_resource *resource,
                                 uint32_t tv_sec_hi, uint32_t tv_sec_lo, uint32_t tv_nsec)
{
    (void) client;
    timing_surface *ts = wl_resource_get_user_data(resource);
    if (ts == NULL) {
        wl_resource_post_error(resource, WP_COMMIT_TIMER_V1_ERROR_SURFACE_DESTROYED,
                               "the surface of this commit timer was destroyed");
    } else if (tv_nsec >= NSEC_PER_SEC) {
        wl_resource_post_error(resource, WP_COMMIT_TIMER_V1_ERROR_INVALID_TIMESTAMP,
                               "tv_nsec is out of range");
    } else if (ts->has_target) {
        wl_resource_post_error(resource, WP_COMMIT_TIMER_V1_ERROR_TIMESTAMP_EXISTS,
                               "a timestamp is already set for this commit");
    } else {
        ts->has_target = true;
        ts->target_ns = timestamp_ns(tv_sec_hi, tv_sec_lo, tv_nsec);
    }
}

static int64_t timestamp_ns(uint32_t tv_sec_hi, uint32_t tv_sec_lo, uint32_t tv_nsec)
{
    uint64_t seconds = (uint64_t) tv_sec_hi << 32 | tv_sec_lo;
    if (seconds >= (uint64_t) (INT64_MAX / NSEC_PER_SEC)) {
        return INT64_MAX;
    }
    return (int64_t) seconds * NSEC_PER_SEC + tv_nsec;
}

static void handle_resource_destroy(struct wl_resource *resource)
{
    timing_surface *ts = wl_resource_get_user_data(resource);
    if (ts != NULL) {
        ts->resource = NULL;
    }
}
