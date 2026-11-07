#include "displays.h"

#include <stdlib.h>
#include <wlr/types/wlr_output.h>

#include "display_batch.h"
#include "display_policy.h"
#include "input/idle.h"
#include "output_config.h"
#include "output_private.h"
#include "output_setting.h"
#include "xfce_output_heads.h"
#include "xfce_outputs.h"

typedef enum placement_pass {
    PLACEMENT_SAVED,
    PLACEMENT_AUTOMATIC,
} placement_pass;

struct displays {
    compositor *server;
    struct wl_event_source *settle_idle;
    bool lid_closed;
};

static void handle_settle_idle(void *data);
static void schedule_settle(displays *displays);
static void adopt_plan(displays *displays, const display_batch *batch);
static void adopt(output_display *display, const display_saved *saved);
static void enforce(const displays *displays, const display_batch *batch);
static void switch_on_pass(const display_batch *batch, const bool *enabled, placement_pass pass);
static void switch_on(output *output);
static void switch_off(output *output);
static void announce(const displays *displays);
static void note_configured(const displays *displays, output *output);
static void save(const displays *displays);
static bool any_enabled(const display_batch *batch);
static void displays_settle(displays *displays);

displays *displays_create(compositor *server)
{
    displays *d = calloc(1, sizeof(*d));
    if (d != NULL) {
        d->server = server;
    }
    return d;
}

void displays_destroy(displays *d)
{
    if (d->settle_idle != NULL) {
        wl_event_source_remove(d->settle_idle);
    }
    free(d);
}

void displays_output_added(displays *d, output *o)
{
    idle_activity(d->server->idle);
    output_display *display = output_display_of(o);
    display->edid = edid_read(output_wlr(o));
    display->is_internal = display_is_internal(output_wlr(o)->name);
    display->is_pending = true;
    display->wants_enabled = true;
    schedule_settle(d);
}

void displays_output_removed(displays *d)
{
    schedule_settle(d);
}

void displays_start(displays *d)
{
    displays_settle(d);
    outputs_arrange(d->server->outputs);
}

static void displays_settle(displays *d)
{
    if (d->settle_idle != NULL) {
        wl_event_source_remove(d->settle_idle);
        d->settle_idle = NULL;
    }
    display_batch batch;
    display_batch_collect(d->server->outputs, &batch);
    if (display_batch_has_pending(&batch)) {
        adopt_plan(d, &batch);
    }
    enforce(d, &batch);
    announce(d);
}

void displays_set_lid_closed(displays *d, bool closed)
{
    if (d->lid_closed == closed) {
        return;
    }
    d->lid_closed = closed;
    display_batch batch;
    display_batch_collect(d->server->outputs, &batch);
    if (display_batch_has_pending(&batch)) {
        schedule_settle(d);
        return;
    }
    enforce(d, &batch);
    announce(d);
}

void displays_configured(displays *d, struct wl_client *client)
{
    display_batch batch;
    display_batch_collect(d->server->outputs, &batch);
    for (size_t i = 0; i < batch.count; i++) {
        note_configured(d, batch.outputs[i]);
    }
    if (!display_batch_has_pending(&batch) &&
        !xfce_output_heads_has_client(d->server->xfce_output_heads, client)) {
        save(d);
    }
}

void displays_reload_primary(displays *d)
{
    display_batch batch;
    display_batch_collect(d->server->outputs, &batch);
    if (batch.count == 0 || display_batch_has_pending(&batch)) {
        return;
    }
    const compositor_services *services = d->server->services;
    display_set set = {batch.states, batch.count};
    display_plan plan = services->load_displays(services->self, &set);
    for (size_t i = 0; i < batch.count; i++) {
        output_display_of(batch.outputs[i])->is_primary = plan.displays[i].is_primary;
    }
    xfce_outputs_refresh(d->server->xfce_outputs);
}

static void handle_settle_idle(void *data)
{
    displays *d = data;
    d->settle_idle = NULL;
    displays_settle(d);
}

static void schedule_settle(displays *d)
{
    if (d->settle_idle == NULL) {
        d->settle_idle = wl_event_loop_add_idle(d->server->loop, handle_settle_idle, d);
    }
}

static void adopt_plan(displays *d, const display_batch *batch)
{
    const compositor_services *services = d->server->services;
    display_set set = {batch->states, batch->count};
    display_plan plan = services->load_displays(services->self, &set);
    for (size_t i = 0; i < batch->count; i++) {
        adopt(output_display_of(batch->outputs[i]), &plan.displays[i]);
    }
}

static void adopt(output_display *display, const display_saved *saved)
{
    display->is_primary = saved->is_primary;
    if (!display->is_pending) {
        return;
    }
    display->wants_enabled = !saved->is_known || saved->setting.enabled;
    display->has_last_enabled = saved->is_known && saved->setting.enabled;
    if (display->has_last_enabled) {
        display->last_enabled = saved->setting;
    }
}

static void enforce(const displays *d, const display_batch *batch)
{
    bool enabled[DISPLAY_MAX] = {false};
    display_wish wishes[DISPLAY_MAX];
    display_wishes in = {wishes, display_batch_wishes(batch, wishes), d->lid_closed};
    display_policy_decide(&in, enabled);
    for (size_t i = 0; i < batch->count; i++) {
        if (!enabled[i]) {
            switch_off(batch->outputs[i]);
        }
    }
    switch_on_pass(batch, enabled, PLACEMENT_SAVED);
    switch_on_pass(batch, enabled, PLACEMENT_AUTOMATIC);
    for (size_t i = 0; i < batch->count; i++) {
        output_display_of(batch->outputs[i])->is_pending = false;
    }
}

static void switch_on_pass(const display_batch *batch, const bool *enabled, placement_pass pass)
{
    for (size_t i = 0; i < batch->count; i++) {
        output *o = batch->outputs[i];
        bool is_saved = output_display_of(o)->has_last_enabled;
        if (enabled[i] && is_saved == (pass == PLACEMENT_SAVED)) {
            switch_on(o);
        }
    }
}

static void switch_on(output *o)
{
    output_display *display = output_display_of(o);
    if (output_is_active(o) && !display->is_pending) {
        return;
    }
    bool placed = display->has_last_enabled ? output_apply_at(o, &display->last_enabled)
                                            : output_apply_auto(o);
    if (placed) {
        output_remember(o);
    }
}

static void switch_off(output *o)
{
    if (output_is_active(o) || output_wlr(o)->enabled) {
        output_turn_off(o);
    }
}

static void announce(const displays *d)
{
    outputs_mark_changed(d->server->outputs);
    outputs_schedule_frames(d->server->outputs);
    output_config_schedule_publish(d->server->output_config);
}

static void note_configured(const displays *d, output *o)
{
    output_display *display = output_display_of(o);
    bool active = output_is_active(o);
    bool hidden_by_lid = d->lid_closed && display->is_internal && !active;
    if (!hidden_by_lid) {
        display->wants_enabled = active;
    }
    output_remember(o);
}

static void save(const displays *d)
{
    display_batch batch;
    display_batch_collect(d->server->outputs, &batch);
    if (!any_enabled(&batch)) {
        return;
    }
    const compositor_services *services = d->server->services;
    display_set set = {batch.states, batch.count};
    services->save_displays(services->self, &set);
}

static bool any_enabled(const display_batch *batch)
{
    for (size_t i = 0; i < batch->count; i++) {
        if (batch->states[i].setting.enabled) {
            return true;
        }
    }
    return false;
}
