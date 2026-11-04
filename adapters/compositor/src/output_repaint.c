#include "output_repaint.h"

#include <time.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_scene.h>

#include "listener.h"
#include "output_private.h"

enum {
    NS_PER_SEC = 1000000000,
    NS_PER_MS = 1000000,
};

static int handle_timer(void *data);
static void handle_present(struct wl_listener *listener, void *data);
static void collect_render_time(output_repaint *repaint);
static int planned_delay(output *output);
static bool can_delay(output *output);
static int64_t render_budget(const output *output);
static bool wants_auto(const output *output);
static bool renderer_can_time(struct wlr_renderer *renderer);
static int64_t timespec_ns(const struct timespec *time);

bool output_repaint_init(output *o)
{
    output_repaint *r = &o->repaint;
    wl_list_init(&r->present.link);
    r->timer = wl_event_loop_add_timer(o->server->loop, handle_timer, o);
    if (r->timer == NULL) {
        return false;
    }
    r->can_time = renderer_can_time(o->server->renderer);
    listener_connect(&r->present, &o->wlr_output->events.present, handle_present);
    return true;
}

void output_repaint_finish(output *o)
{
    output_repaint *r = &o->repaint;
    listener_disconnect(&r->present);
    if (r->timer != NULL) {
        wl_event_source_remove(r->timer);
    }
    wlr_scene_timer_finish(&r->render_timer);
}

void output_repaint_frame(output *o)
{
    int delay = planned_delay(o);
    if (delay == 0) {
        output_render(o);
        return;
    }
    o->wlr_output->frame_pending = true;
    wl_event_source_timer_update(o->repaint.timer, delay);
    output_send_frame_done(o);
}

struct wlr_scene_timer *output_repaint_timer(output *o)
{
    return wants_auto(o) ? &o->repaint.render_timer : NULL;
}

static int handle_timer(void *data)
{
    output *o = data;
    o->wlr_output->frame_pending = false;
    output_render_frame(o);
    return 0;
}

static void handle_present(struct wl_listener *listener, void *data)
{
    output_repaint *r = wl_container_of(listener, r, present);
    const struct wlr_output_event_present *event = data;
    if (!event->presented) {
        return;
    }
    r->vblank = (vblank_prediction) {timespec_ns(&event->when), event->refresh};
    r->predictable = (event->flags & WLR_OUTPUT_PRESENT_VSYNC) != 0;
    collect_render_time(r);
}

static void collect_render_time(output_repaint *r)
{
    if (r->render_timer.render_timer == NULL) {
        return;
    }
    int64_t duration = wlr_scene_timer_get_duration_ns(&r->render_timer);
    if (duration >= 0) {
        render_history_add(&r->history, duration);
    }
    wlr_scene_timer_finish(&r->render_timer);
    r->render_timer = (struct wlr_scene_timer) {0};
}

static int planned_delay(output *o)
{
    if (!can_delay(o)) {
        return 0;
    }
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    repaint_plan plan = {timespec_ns(&now), o->repaint.vblank, render_budget(o)};
    return repaint_delay_ms(&plan);
}

static bool can_delay(output *o)
{
    const struct wlr_output *w = o->wlr_output;
    return o->repaint.predictable &&
           w->adaptive_sync_status != WLR_OUTPUT_ADAPTIVE_SYNC_ENABLED &&
           !output_wants_tearing(o);
}

static int64_t render_budget(const output *o)
{
    wm_render_time setting = o->server->config.compositing.max_render_time;
    if (setting.mode == WM_RENDER_TIME_FIXED) {
        return (int64_t) setting.ms * NS_PER_MS;
    }
    if (wants_auto(o)) {
        return render_history_budget(&o->repaint.history);
    }
    return REPAINT_NO_BUDGET;
}

static bool wants_auto(const output *o)
{
    return o->server->config.compositing.max_render_time.mode == WM_RENDER_TIME_AUTO &&
           o->repaint.can_time;
}

static bool renderer_can_time(struct wlr_renderer *renderer)
{
    struct wlr_render_timer *probe = wlr_render_timer_create(renderer);
    if (probe == NULL) {
        return false;
    }
    wlr_render_timer_destroy(probe);
    return true;
}

static int64_t timespec_ns(const struct timespec *time)
{
    return (int64_t) time->tv_sec * NS_PER_SEC + time->tv_nsec;
}
