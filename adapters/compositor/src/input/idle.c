#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_idle_inhibit_v1.h>
#include <wlr/types/wlr_idle_notify_v1.h>

#include "util/clock.h"
#include "compositor/services.h"
#include "idle.h"
#include "util/listener.h"
#include "output/output.h"
#include "output/output_power.h"

enum {
    ASLEEP_OUTPUTS_MAX = 16,
};

typedef struct inhibitor_watch {
    idle *owner;
    struct wl_listener destroy;
    struct wl_listener map;
    struct wl_listener unmap;
} inhibitor_watch;

struct idle {
    compositor *server;
    struct wl_event_source *refresh;
    struct wl_event_source *display_timer;
    uint32_t display_off_ms;
    int64_t last_activity_ms;
    bool displays_asleep;
    bool surface_inhibited;
    bool service_inhibited;
    bool inhibited;
    char *asleep_outputs[ASLEEP_OUTPUTS_MAX];
    size_t asleep_count;
    struct wl_listener new_inhibitor;
};

static void handle_new_inhibitor(struct wl_listener *listener, void *data);
static void handle_inhibitor_destroy(struct wl_listener *listener, void *data);
static void handle_surface_map(struct wl_listener *listener, void *data);
static void handle_surface_unmap(struct wl_listener *listener, void *data);
static void handle_refresh(void *data);
static bool any_visible_inhibitor(const struct wlr_idle_inhibit_manager_v1 *manager);
static int handle_display_timer(void *data);
static void update_inhibited(idle *idle);
static void sleep_displays(idle *idle);
static void wake_displays(idle *idle);
static void arm_display_timer(idle *idle, int64_t delay_ms);

idle *idle_create(compositor *server)
{
    idle *i = calloc(1, sizeof(*i));
    if (i == NULL) {
        return NULL;
    }
    i->server = server;
    i->last_activity_ms = clock_now_ms();
    i->display_timer = wl_event_loop_add_timer(server->loop, handle_display_timer, i);
    wl_list_init(&i->new_inhibitor.link);
    if (server->globals.idle_inhibit != NULL) {
        listener_connect(&i->new_inhibitor, &server->globals.idle_inhibit->events.new_inhibitor,
                         handle_new_inhibitor);
    }
    return i;
}

void idle_destroy(idle *i)
{
    listener_disconnect(&i->new_inhibitor);
    if (i->refresh != NULL) {
        wl_event_source_remove(i->refresh);
    }
    if (i->display_timer != NULL) {
        wl_event_source_remove(i->display_timer);
    }
    for (size_t index = 0; index < i->asleep_count; index++) {
        free(i->asleep_outputs[index]);
    }
    free(i);
}

void idle_activity(idle *i)
{
    if (i == NULL) {
        return;
    }
    i->last_activity_ms = clock_now_ms();
    if (i->displays_asleep) {
        wake_displays(i);
        arm_display_timer(i, i->display_off_ms);
    }
}

void idle_reload_display_off(idle *i)
{
    if (i == NULL) {
        return;
    }
    const compositor_services *services = i->server->services;
    uint32_t previous_ms = i->display_off_ms;
    i->display_off_ms =
        services->display_off_ms != NULL ? services->display_off_ms(services->self) : 0;
    if (previous_ms == 0 && i->display_off_ms != 0) {
        i->last_activity_ms = clock_now_ms();
    }
    i->service_inhibited =
        services->idle_inhibited != NULL && services->idle_inhibited(services->self);
    update_inhibited(i);
    if (i->displays_asleep && i->display_off_ms == 0) {
        wake_displays(i);
    }
    arm_display_timer(i, i->display_off_ms);
}

void idle_schedule_refresh(idle *i)
{
    if (i != NULL && i->refresh == NULL) {
        i->refresh = wl_event_loop_add_idle(i->server->loop, handle_refresh, i);
    }
}

static void handle_new_inhibitor(struct wl_listener *listener, void *data)
{
    idle *i = wl_container_of(listener, i, new_inhibitor);
    struct wlr_idle_inhibitor_v1 *inhibitor = data;
    inhibitor_watch *watch = calloc(1, sizeof(*watch));
    if (watch != NULL) {
        watch->owner = i;
        listener_connect(&watch->destroy, &inhibitor->events.destroy, handle_inhibitor_destroy);
        listener_connect(&watch->map, &inhibitor->surface->events.map, handle_surface_map);
        listener_connect(&watch->unmap, &inhibitor->surface->events.unmap, handle_surface_unmap);
    }
    idle_schedule_refresh(i);
}

static void handle_inhibitor_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    inhibitor_watch *watch = wl_container_of(listener, watch, destroy);
    idle_schedule_refresh(watch->owner);
    listener_disconnect(&watch->destroy);
    listener_disconnect(&watch->map);
    listener_disconnect(&watch->unmap);
    free(watch);
}

static void handle_surface_map(struct wl_listener *listener, void *data)
{
    (void) data;
    inhibitor_watch *watch = wl_container_of(listener, watch, map);
    idle_schedule_refresh(watch->owner);
}

static void handle_surface_unmap(struct wl_listener *listener, void *data)
{
    (void) data;
    inhibitor_watch *watch = wl_container_of(listener, watch, unmap);
    idle_schedule_refresh(watch->owner);
}

static void handle_refresh(void *data)
{
    idle *i = data;
    i->refresh = NULL;
    const protocol_globals *globals = &i->server->globals;
    i->surface_inhibited =
        globals->idle_inhibit != NULL && any_visible_inhibitor(globals->idle_inhibit);
    update_inhibited(i);
}

static void update_inhibited(idle *i)
{
    bool was_inhibited = i->inhibited;
    i->inhibited = i->surface_inhibited || i->service_inhibited;
    if (i->server->globals.idle_notifier != NULL) {
        wlr_idle_notifier_v1_set_inhibited(i->server->globals.idle_notifier, i->inhibited);
    }
    if (was_inhibited && !i->inhibited && !i->displays_asleep) {
        i->last_activity_ms = clock_now_ms();
        arm_display_timer(i, i->display_off_ms);
    }
}

static bool any_visible_inhibitor(const struct wlr_idle_inhibit_manager_v1 *manager)
{
    struct wlr_idle_inhibitor_v1 *inhibitor;
    wl_list_for_each(inhibitor, &manager->inhibitors, link) {
        struct wlr_surface *surface = inhibitor->surface;
        if (surface->mapped && !wl_list_empty(&surface->current_outputs)) {
            return true;
        }
    }
    return false;
}

static int handle_display_timer(void *data)
{
    idle *i = data;
    if (i->display_off_ms == 0 || i->displays_asleep) {
        return 0;
    }
    int64_t idle_ms = clock_now_ms() - i->last_activity_ms;
    if (idle_ms < i->display_off_ms) {
        arm_display_timer(i, i->display_off_ms - idle_ms);
    } else if (i->inhibited) {
        arm_display_timer(i, i->display_off_ms);
    } else {
        sleep_displays(i);
    }
    return 0;
}

static void sleep_displays(idle *i)
{
    i->displays_asleep = true;
    for (size_t index = 0; index < outputs_count(i->server->outputs); index++) {
        output *o = outputs_at_index(i->server->outputs, index);
        struct wlr_output *wlr_output = output_wlr(o);
        if (!output_is_active(o) || !wlr_output->enabled || i->asleep_count == ASLEEP_OUTPUTS_MAX) {
            continue;
        }
        i->asleep_outputs[i->asleep_count++] = strdup(wlr_output->name);
        output_power_set(i->server, o, false);
    }
}

static void wake_displays(idle *i)
{
    i->displays_asleep = false;
    for (size_t index = 0; index < outputs_count(i->server->outputs); index++) {
        output *o = outputs_at_index(i->server->outputs, index);
        for (size_t slept = 0; slept < i->asleep_count; slept++) {
            if (i->asleep_outputs[slept] != NULL
                && strcmp(i->asleep_outputs[slept], output_wlr(o)->name) == 0) {
                output_power_set(i->server, o, true);
            }
        }
    }
    for (size_t slept = 0; slept < i->asleep_count; slept++) {
        free(i->asleep_outputs[slept]);
    }
    i->asleep_count = 0;
}

static void arm_display_timer(idle *i, int64_t delay_ms)
{
    if (i->display_timer != NULL) {
        int delay = delay_ms > INT_MAX ? INT_MAX : (int) delay_ms;
        wl_event_source_timer_update(i->display_timer, delay);
    }
}
