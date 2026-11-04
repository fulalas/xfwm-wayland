#include <wlr/types/wlr_foreign_toplevel_management_v1.h>

#include "foreign_toplevel_private.h"
#include "listener.h"

static void handle_maximize(struct wl_listener *listener, void *data);
static void handle_minimize(struct wl_listener *listener, void *data);
static void handle_activate(struct wl_listener *listener, void *data);
static void handle_fullscreen(struct wl_listener *listener, void *data);
static void handle_close(struct wl_listener *listener, void *data);
static void activate(toplevel_entry *entry);

void toplevel_requests_connect(toplevel_entry *entry)
{
    struct wlr_foreign_toplevel_handle_v1 *handle = entry->wlr;
    toplevel_requests *r = &entry->requests;
    listener_connect(&r->maximize, &handle->events.request_maximize, handle_maximize);
    listener_connect(&r->minimize, &handle->events.request_minimize, handle_minimize);
    listener_connect(&r->activate, &handle->events.request_activate, handle_activate);
    listener_connect(&r->fullscreen, &handle->events.request_fullscreen, handle_fullscreen);
    listener_connect(&r->close, &handle->events.request_close, handle_close);
}

void toplevel_requests_disconnect(toplevel_entry *entry)
{
    toplevel_requests *r = &entry->requests;
    listener_disconnect(&r->maximize);
    listener_disconnect(&r->minimize);
    listener_disconnect(&r->activate);
    listener_disconnect(&r->fullscreen);
    listener_disconnect(&r->close);
}

static void handle_maximize(struct wl_listener *listener, void *data)
{
    toplevel_entry *entry = wl_container_of(listener, entry, requests.maximize);
    const struct wlr_foreign_toplevel_handle_v1_maximized_event *event = data;
    wm_state_change change = view_state_change(WM_STATE_MAXIMIZED, event->maximized);
    view_request_state(entry->view, &change);
}

static void handle_minimize(struct wl_listener *listener, void *data)
{
    toplevel_entry *entry = wl_container_of(listener, entry, requests.minimize);
    const struct wlr_foreign_toplevel_handle_v1_minimized_event *event = data;
    if (!event->minimized) {
        activate(entry);
        return;
    }
    wm_state_change change = view_state_change(WM_STATE_MINIMIZED, true);
    view_request_state(entry->view, &change);
}

static void handle_activate(struct wl_listener *listener, void *data)
{
    (void) data;
    toplevel_entry *entry = wl_container_of(listener, entry, requests.activate);
    activate(entry);
}

static void handle_fullscreen(struct wl_listener *listener, void *data)
{
    toplevel_entry *entry = wl_container_of(listener, entry, requests.fullscreen);
    const struct wlr_foreign_toplevel_handle_v1_fullscreen_event *event = data;
    wm_state_change change = view_state_change(WM_STATE_FULLSCREEN, event->fullscreen);
    view_request_state(entry->view, &change);
}

static void handle_close(struct wl_listener *listener, void *data)
{
    (void) data;
    toplevel_entry *entry = wl_container_of(listener, entry, requests.close);
    wm_window *window = view_window(entry->view);
    if (window != NULL) {
        wm_request_close(entry->owner->server->desktop, window);
    }
}

static void activate(toplevel_entry *entry)
{
    wm_window *window = view_window(entry->view);
    if (window != NULL) {
        wm_request_activate(entry->owner->server->desktop, window, WM_ACTIVATION_PAGER);
    }
}
