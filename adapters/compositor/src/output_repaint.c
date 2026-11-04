#include "output_repaint.h"

#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_scene.h>

#include "clock.h"
#include "output_private.h"
#include "present_sync.h"

static int handle_timer(void *data);
static void collect_render_time(output_repaint *repaint);
static int planned_delay(output *output);
static int64_t render_budget(const output *output);
static bool wants_auto(const output *output);
static void drop_render_timer(output_repaint *repaint);

bool output_repaint_init(output *o)
{
    output_repaint *r = &o->repaint;
    r->timer = wl_event_loop_add_timer(o->server->loop, handle_timer, o);
    return r->timer != NULL;
}

void output_repaint_finish(output *o)
{
    wl_event_source_remove(o->repaint.timer);
    wlr_scene_timer_finish(&o->repaint.render_timer);
}

void output_repaint_frame(output *o)
{
    output_repaint *r = &o->repaint;
    if (r->armed) {
        return;
    }
    collect_render_time(r);
    int delay = planned_delay(o);
    if (delay == 0) {
        output_render(o);
        return;
    }
    r->armed = true;
    r->wake_due_ns = clock_now_ns() + (int64_t) delay * CLOCK_NS_PER_MS;
    wl_event_source_timer_update(r->timer, delay);
    output_send_frame_done(o);
}

struct wlr_scene_timer *output_repaint_timer(output *o)
{
    return wants_auto(o) ? &o->repaint.render_timer : NULL;
}

bool output_repaint_renderer_can_time(struct wlr_renderer *renderer)
{
    struct wlr_render_timer *probe = wlr_render_timer_create(renderer);
    if (probe == NULL) {
        return false;
    }
    wlr_render_timer_destroy(probe);
    return true;
}

void output_repaint_reset_timing(output *o)
{
    drop_render_timer(&o->repaint);
    o->repaint.history = (render_history) {0};
}

static int handle_timer(void *data)
{
    output *o = data;
    output_repaint *r = &o->repaint;
    int64_t lateness = clock_now_ns() - r->wake_due_ns;
    r->wake_lateness_ns = lateness > 0 ? lateness : 0;
    r->armed = false;
    output_render_frame(o);
    return 0;
}

static void collect_render_time(output_repaint *r)
{
    int64_t lateness = r->wake_lateness_ns;
    r->wake_lateness_ns = 0;
    if (r->render_timer.render_timer == NULL) {
        return;
    }
    int64_t duration = wlr_scene_timer_get_duration_ns(&r->render_timer);
    if (duration >= 0) {
        render_history_add(&r->history, duration + lateness);
    }
    drop_render_timer(r);
}

static int planned_delay(output *o)
{
    const vblank_clock *clock = present_sync_clock(o->server->present_sync, o->wlr_output);
    if (clock == NULL || output_wants_tearing(o)) {
        return 0;
    }
    repaint_plan plan = {clock_now_ns(), *clock, render_budget(o)};
    return repaint_delay_ms(&plan);
}

static int64_t render_budget(const output *o)
{
    wm_render_time setting = o->server->config.compositing.max_render_time;
    if (setting.mode == WM_RENDER_TIME_FIXED) {
        return (int64_t) setting.ms * CLOCK_NS_PER_MS;
    }
    if (wants_auto(o)) {
        return render_history_budget(&o->repaint.history);
    }
    return REPAINT_NO_BUDGET;
}

static bool wants_auto(const output *o)
{
    return o->server->config.compositing.max_render_time.mode == WM_RENDER_TIME_AUTO &&
           o->server->outputs->can_time_renders;
}

static void drop_render_timer(output_repaint *r)
{
    wlr_scene_timer_finish(&r->render_timer);
    r->render_timer = (struct wlr_scene_timer) {0};
}
