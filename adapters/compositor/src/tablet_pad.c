#include <libinput.h>
#include <stdlib.h>
#include <wlr/backend/libinput.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_tablet_pad.h>
#include <wlr/types/wlr_tablet_tool.h>
#include <wlr/types/wlr_tablet_v2.h>
#include <wlr/util/log.h>

#include "listener.h"
#include "tablet_private.h"

typedef struct tablet_pad {
    tablet_input *owner;
    struct wlr_tablet_pad *pad;
    struct wlr_tablet_v2_tablet_pad *v2;
    struct wlr_surface *focus;
    struct wl_list link;
    struct wl_listener button;
    struct wl_listener ring;
    struct wl_listener strip;
    struct wl_listener destroy;
    struct wl_listener focus_destroy;
} tablet_pad;

static void handle_button(struct wl_listener *listener, void *data);
static void handle_ring(struct wl_listener *listener, void *data);
static void handle_strip(struct wl_listener *listener, void *data);
static void handle_destroy(struct wl_listener *listener, void *data);
static void handle_focus_destroy(struct wl_listener *listener, void *data);
static void free_pad(tablet_pad *pad);
static void focus_pad(tablet_pad *pad, struct wlr_surface *surface);
static void drop_focus(tablet_pad *pad);
static tablet_device *paired_tablet(const tablet_pad *pad);
static bool same_group(struct wlr_input_device *a, struct wlr_input_device *b);

void tablet_pads_add(tablet_input *tablets, struct wlr_input_device *device)
{
    tablet_pad *pad = calloc(1, sizeof(*pad));
    if (pad == NULL) {
        return;
    }
    pad->owner = tablets;
    pad->pad = wlr_tablet_pad_from_input_device(device);
    wl_list_init(&pad->focus_destroy.link);
    listener_connect(&pad->destroy, &device->events.destroy, handle_destroy);
    pad->v2 = wlr_tablet_pad_create(tablets->manager, tablets->seat->wlr_seat, device);
    if (pad->v2 == NULL) {
        wlr_log(WLR_ERROR, "cannot announce tablet pad %s to clients", device->name);
    }
    listener_connect(&pad->button, &pad->pad->events.button, handle_button);
    listener_connect(&pad->ring, &pad->pad->events.ring, handle_ring);
    listener_connect(&pad->strip, &pad->pad->events.strip, handle_strip);
    wl_list_insert(&tablets->pads, &pad->link);
    focus_pad(pad, tablets->seat->wlr_seat->keyboard_state.focused_surface);
}

void tablet_pads_destroy(tablet_input *tablets)
{
    tablet_pad *pad;
    tablet_pad *tmp;
    wl_list_for_each_safe(pad, tmp, &tablets->pads, link) {
        free_pad(pad);
    }
}

void tablet_pads_focus(tablet_input *tablets, struct wlr_surface *surface)
{
    tablet_pad *pad;
    wl_list_for_each(pad, &tablets->pads, link) {
        focus_pad(pad, surface);
    }
}

void tablet_pads_tablet_added(tablet_input *tablets)
{
    tablet_pads_focus(tablets, tablets->seat->wlr_seat->keyboard_state.focused_surface);
}

static void handle_button(struct wl_listener *listener, void *data)
{
    tablet_pad *pad = wl_container_of(listener, pad, button);
    const struct wlr_tablet_pad_button_event *event = data;
    seat_notify_activity(pad->owner->seat);
    if (pad->v2 == NULL) {
        return;
    }
    wlr_tablet_v2_tablet_pad_notify_mode(pad->v2, event->group, event->mode, event->time_msec);
    wlr_tablet_v2_tablet_pad_notify_button(pad->v2, event->button, event->time_msec,
                                           event->state == WLR_BUTTON_PRESSED
                                               ? ZWP_TABLET_PAD_V2_BUTTON_STATE_PRESSED
                                               : ZWP_TABLET_PAD_V2_BUTTON_STATE_RELEASED);
}

static void handle_ring(struct wl_listener *listener, void *data)
{
    tablet_pad *pad = wl_container_of(listener, pad, ring);
    const struct wlr_tablet_pad_ring_event *event = data;
    seat_notify_activity(pad->owner->seat);
    if (pad->v2 != NULL) {
        bool finger = event->source == WLR_TABLET_PAD_RING_SOURCE_FINGER;
        wlr_tablet_v2_tablet_pad_notify_ring(pad->v2, event->ring, event->position, finger,
                                             event->time_msec);
    }
}

static void handle_strip(struct wl_listener *listener, void *data)
{
    tablet_pad *pad = wl_container_of(listener, pad, strip);
    const struct wlr_tablet_pad_strip_event *event = data;
    seat_notify_activity(pad->owner->seat);
    if (pad->v2 != NULL) {
        bool finger = event->source == WLR_TABLET_PAD_STRIP_SOURCE_FINGER;
        wlr_tablet_v2_tablet_pad_notify_strip(pad->v2, event->strip, event->position, finger,
                                              event->time_msec);
    }
}

static void handle_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    tablet_pad *pad = wl_container_of(listener, pad, destroy);
    free_pad(pad);
}

static void handle_focus_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    tablet_pad *pad = wl_container_of(listener, pad, focus_destroy);
    listener_disconnect(&pad->focus_destroy);
    pad->focus = NULL;
}

static void free_pad(tablet_pad *pad)
{
    listener_disconnect(&pad->focus_destroy);
    listener_disconnect(&pad->button);
    listener_disconnect(&pad->ring);
    listener_disconnect(&pad->strip);
    listener_disconnect(&pad->destroy);
    wl_list_remove(&pad->link);
    free(pad);
}

static void focus_pad(tablet_pad *pad, struct wlr_surface *surface)
{
    tablet_device *tablet = paired_tablet(pad);
    if (pad->v2 == NULL || pad->focus == surface) {
        return;
    }
    drop_focus(pad);
    if (surface == NULL || tablet == NULL || tablet->v2 == NULL ||
        !wlr_surface_accepts_tablet_v2(surface, tablet->v2)) {
        return;
    }
    wlr_tablet_v2_tablet_pad_notify_enter(pad->v2, tablet->v2, surface);
    pad->focus = surface;
    listener_connect(&pad->focus_destroy, &surface->events.destroy, handle_focus_destroy);
}

static void drop_focus(tablet_pad *pad)
{
    if (pad->focus == NULL) {
        return;
    }
    wlr_tablet_v2_tablet_pad_notify_leave(pad->v2, pad->focus);
    listener_disconnect(&pad->focus_destroy);
    pad->focus = NULL;
}

static tablet_device *paired_tablet(const tablet_pad *pad)
{
    tablet_device *tablet;
    tablet_device *fallback = NULL;
    wl_list_for_each(tablet, &pad->owner->tablets, link) {
        if (same_group(&pad->pad->base, &tablet->tablet->base)) {
            return tablet;
        }
        fallback = tablet;
    }
    return fallback;
}

static bool same_group(struct wlr_input_device *a, struct wlr_input_device *b)
{
    if (!wlr_input_device_is_libinput(a) || !wlr_input_device_is_libinput(b)) {
        return false;
    }
    struct libinput_device *first = wlr_libinput_get_device_handle(a);
    struct libinput_device *second = wlr_libinput_get_device_handle(b);
    return libinput_device_get_device_group(first) == libinput_device_get_device_group(second);
}
