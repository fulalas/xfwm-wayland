#include <stdlib.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_primary_selection.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>

#include "listener.h"
#include "scene_layers.h"
#include "seat_private.h"

struct selection {
    seat *seat;
    struct wl_listener request_set_selection;
    struct wl_listener request_set_primary;
    struct wl_listener request_start_drag;
    struct wl_listener start_drag;
};

static void handle_request_set_selection(struct wl_listener *listener, void *data);
static void handle_request_set_primary(struct wl_listener *listener, void *data);
static void handle_request_start_drag(struct wl_listener *listener, void *data);
static void handle_start_drag(struct wl_listener *listener, void *data);
static struct wlr_scene_tree *drag_tree(const selection *selection);

selection *selection_create(seat *s)
{
    selection *sel = calloc(1, sizeof(*sel));
    if (sel == NULL) {
        return NULL;
    }
    sel->seat = s;
    struct wlr_seat *wlr_seat = s->wlr_seat;
    listener_connect(&sel->request_set_selection, &wlr_seat->events.request_set_selection,
                     handle_request_set_selection);
    listener_connect(&sel->request_set_primary, &wlr_seat->events.request_set_primary_selection,
                     handle_request_set_primary);
    listener_connect(&sel->request_start_drag, &wlr_seat->events.request_start_drag,
                     handle_request_start_drag);
    listener_connect(&sel->start_drag, &wlr_seat->events.start_drag, handle_start_drag);
    return sel;
}

void selection_destroy(selection *sel)
{
    listener_disconnect(&sel->request_set_selection);
    listener_disconnect(&sel->request_set_primary);
    listener_disconnect(&sel->request_start_drag);
    listener_disconnect(&sel->start_drag);
    free(sel);
}

void selection_move_icons(selection *sel)
{
    struct wlr_scene_tree *tree = drag_tree(sel);
    if (wl_list_empty(&tree->children)) {
        return;
    }
    struct wlr_cursor *cursor = sel->seat->cursor;
    wlr_scene_node_set_position(&tree->node, (int) cursor->x, (int) cursor->y);
}

static void handle_request_set_selection(struct wl_listener *listener, void *data)
{
    selection *sel = wl_container_of(listener, sel, request_set_selection);
    const struct wlr_seat_request_set_selection_event *event = data;
    wlr_seat_set_selection(sel->seat->wlr_seat, event->source, event->serial);
}

static void handle_request_set_primary(struct wl_listener *listener, void *data)
{
    selection *sel = wl_container_of(listener, sel, request_set_primary);
    const struct wlr_seat_request_set_primary_selection_event *event = data;
    wlr_seat_set_primary_selection(sel->seat->wlr_seat, event->source, event->serial);
}

static void handle_request_start_drag(struct wl_listener *listener, void *data)
{
    selection *sel = wl_container_of(listener, sel, request_start_drag);
    const struct wlr_seat_request_start_drag_event *event = data;
    struct wlr_seat *wlr_seat = sel->seat->wlr_seat;
    if (wlr_seat_validate_pointer_grab_serial(wlr_seat, event->origin, event->serial)) {
        wlr_seat_start_pointer_drag(wlr_seat, event->drag, event->serial);
        return;
    }
    struct wlr_touch_point *point;
    if (wlr_seat_validate_touch_grab_serial(wlr_seat, event->origin, event->serial, &point)) {
        wlr_seat_start_touch_drag(wlr_seat, event->drag, event->serial, point);
        return;
    }
    if (event->drag->source != NULL) {
        wlr_data_source_destroy(event->drag->source);
    }
}

static void handle_start_drag(struct wl_listener *listener, void *data)
{
    selection *sel = wl_container_of(listener, sel, start_drag);
    struct wlr_drag *drag = data;
    if (drag->icon != NULL) {
        wlr_scene_drag_icon_create(drag_tree(sel), drag->icon);
    }
    selection_move_icons(sel);
}

static struct wlr_scene_tree *drag_tree(const selection *sel)
{
    return scene_layers_tree(sel->seat->server->layers, SCENE_LAYER_DRAG);
}
