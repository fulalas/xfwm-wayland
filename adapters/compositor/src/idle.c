#include <stdlib.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_idle_inhibit_v1.h>
#include <wlr/types/wlr_idle_notify_v1.h>

#include "idle.h"
#include "listener.h"

typedef struct inhibitor_watch {
    idle *owner;
    struct wl_listener destroy;
} inhibitor_watch;

struct idle {
    compositor *server;
    struct wl_event_source *refresh;
    struct wl_listener new_inhibitor;
};

static void handle_new_inhibitor(struct wl_listener *listener, void *data);
static void handle_inhibitor_destroy(struct wl_listener *listener, void *data);
static void handle_refresh(void *data);
static bool any_visible_inhibitor(const struct wlr_idle_inhibit_manager_v1 *manager);

idle *idle_create(compositor *server)
{
    idle *i = calloc(1, sizeof(*i));
    if (i == NULL) {
        return NULL;
    }
    i->server = server;
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
    free(i);
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
    }
    idle_schedule_refresh(i);
}

static void handle_inhibitor_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    inhibitor_watch *watch = wl_container_of(listener, watch, destroy);
    idle_schedule_refresh(watch->owner);
    listener_disconnect(&watch->destroy);
    free(watch);
}

static void handle_refresh(void *data)
{
    idle *i = data;
    i->refresh = NULL;
    protocol_globals *globals = &i->server->globals;
    if (globals->idle_notifier != NULL && globals->idle_inhibit != NULL) {
        wlr_idle_notifier_v1_set_inhibited(globals->idle_notifier,
                                           any_visible_inhibitor(globals->idle_inhibit));
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
