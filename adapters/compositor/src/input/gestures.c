#include <stdlib.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_pointer_gestures_v1.h>

#include "util/listener.h"
#include "seat_private.h"

struct gestures {
    seat *seat;
    struct wlr_pointer_gestures_v1 *protocol;
    struct wl_listener swipe_begin;
    struct wl_listener swipe_update;
    struct wl_listener swipe_end;
    struct wl_listener pinch_begin;
    struct wl_listener pinch_update;
    struct wl_listener pinch_end;
    struct wl_listener hold_begin;
    struct wl_listener hold_end;
};

static void handle_swipe_begin(struct wl_listener *listener, void *data);
static void handle_swipe_update(struct wl_listener *listener, void *data);
static void handle_swipe_end(struct wl_listener *listener, void *data);
static void handle_pinch_begin(struct wl_listener *listener, void *data);
static void handle_pinch_update(struct wl_listener *listener, void *data);
static void handle_pinch_end(struct wl_listener *listener, void *data);
static void handle_hold_begin(struct wl_listener *listener, void *data);
static void handle_hold_end(struct wl_listener *listener, void *data);

gestures *gestures_create(seat *s)
{
    gestures *g = calloc(1, sizeof(*g));
    if (g == NULL) {
        return NULL;
    }
    g->seat = s;
    g->protocol = s->server->globals.gestures;
    struct wlr_cursor *cursor = s->cursor;
    listener_connect(&g->swipe_begin, &cursor->events.swipe_begin, handle_swipe_begin);
    listener_connect(&g->swipe_update, &cursor->events.swipe_update, handle_swipe_update);
    listener_connect(&g->swipe_end, &cursor->events.swipe_end, handle_swipe_end);
    listener_connect(&g->pinch_begin, &cursor->events.pinch_begin, handle_pinch_begin);
    listener_connect(&g->pinch_update, &cursor->events.pinch_update, handle_pinch_update);
    listener_connect(&g->pinch_end, &cursor->events.pinch_end, handle_pinch_end);
    listener_connect(&g->hold_begin, &cursor->events.hold_begin, handle_hold_begin);
    listener_connect(&g->hold_end, &cursor->events.hold_end, handle_hold_end);
    return g;
}

void gestures_destroy(gestures *g)
{
    listener_disconnect(&g->swipe_begin);
    listener_disconnect(&g->swipe_update);
    listener_disconnect(&g->swipe_end);
    listener_disconnect(&g->pinch_begin);
    listener_disconnect(&g->pinch_update);
    listener_disconnect(&g->pinch_end);
    listener_disconnect(&g->hold_begin);
    listener_disconnect(&g->hold_end);
    free(g);
}

static void handle_swipe_begin(struct wl_listener *listener, void *data)
{
    gestures *g = wl_container_of(listener, g, swipe_begin);
    const struct wlr_pointer_swipe_begin_event *event = data;
    seat_notify_activity(g->seat);
    wlr_pointer_gestures_v1_send_swipe_begin(g->protocol, g->seat->wlr_seat, event->time_msec,
                                             event->fingers);
}

static void handle_swipe_update(struct wl_listener *listener, void *data)
{
    gestures *g = wl_container_of(listener, g, swipe_update);
    const struct wlr_pointer_swipe_update_event *event = data;
    wlr_pointer_gestures_v1_send_swipe_update(g->protocol, g->seat->wlr_seat, event->time_msec,
                                              event->dx, event->dy);
}

static void handle_swipe_end(struct wl_listener *listener, void *data)
{
    gestures *g = wl_container_of(listener, g, swipe_end);
    const struct wlr_pointer_swipe_end_event *event = data;
    wlr_pointer_gestures_v1_send_swipe_end(g->protocol, g->seat->wlr_seat, event->time_msec,
                                           event->cancelled);
}

static void handle_pinch_begin(struct wl_listener *listener, void *data)
{
    gestures *g = wl_container_of(listener, g, pinch_begin);
    const struct wlr_pointer_pinch_begin_event *event = data;
    seat_notify_activity(g->seat);
    wlr_pointer_gestures_v1_send_pinch_begin(g->protocol, g->seat->wlr_seat, event->time_msec,
                                             event->fingers);
}

static void handle_pinch_update(struct wl_listener *listener, void *data)
{
    gestures *g = wl_container_of(listener, g, pinch_update);
    const struct wlr_pointer_pinch_update_event *event = data;
    wlr_pointer_gestures_v1_send_pinch_update(g->protocol, g->seat->wlr_seat, event->time_msec,
                                              event->dx, event->dy, event->scale,
                                              event->rotation);
}

static void handle_pinch_end(struct wl_listener *listener, void *data)
{
    gestures *g = wl_container_of(listener, g, pinch_end);
    const struct wlr_pointer_pinch_end_event *event = data;
    wlr_pointer_gestures_v1_send_pinch_end(g->protocol, g->seat->wlr_seat, event->time_msec,
                                           event->cancelled);
}

static void handle_hold_begin(struct wl_listener *listener, void *data)
{
    gestures *g = wl_container_of(listener, g, hold_begin);
    const struct wlr_pointer_hold_begin_event *event = data;
    seat_notify_activity(g->seat);
    wlr_pointer_gestures_v1_send_hold_begin(g->protocol, g->seat->wlr_seat, event->time_msec,
                                            event->fingers);
}

static void handle_hold_end(struct wl_listener *listener, void *data)
{
    gestures *g = wl_container_of(listener, g, hold_end);
    const struct wlr_pointer_hold_end_event *event = data;
    wlr_pointer_gestures_v1_send_hold_end(g->protocol, g->seat->wlr_seat, event->time_msec,
                                          event->cancelled);
}
