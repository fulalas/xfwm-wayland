#include <stdlib.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_primary_selection.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>

#include "drag_actions.h"
#include "util/listener.h"
#include "render/scene_layers.h"
#include "seat_private.h"

struct selection {
    seat *seat;
    struct wl_listener request_set_selection;
    struct wl_listener request_set_primary;
    struct wl_listener request_start_drag;
    struct wl_listener start_drag;
    struct wl_listener drag_drop;
    struct wl_listener drag_destroy;
    int32_t offered_actions;
};

static void handle_request_set_selection(struct wl_listener *listener, void *data);
static void handle_request_set_primary(struct wl_listener *listener, void *data);
static void handle_request_start_drag(struct wl_listener *listener, void *data);
static void handle_start_drag(struct wl_listener *listener, void *data);
static void handle_drag_drop(struct wl_listener *listener, void *data);
static void handle_drag_destroy(struct wl_listener *listener, void *data);
static void offer_actions(selection *selection, struct wlr_drag *drag, uint32_t button);
static void keep_target_after_drop(struct wlr_drag *drag);
static void refresh_offers(const struct wlr_drag *drag);
static void refresh_offer(struct wlr_data_offer *offer);
static drag_choice choice_for(const struct wlr_data_offer *offer);
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
    wl_list_init(&sel->drag_drop.link);
    wl_list_init(&sel->drag_destroy.link);
    return sel;
}

void selection_destroy(selection *sel)
{
    listener_disconnect(&sel->request_set_selection);
    listener_disconnect(&sel->request_set_primary);
    listener_disconnect(&sel->request_start_drag);
    listener_disconnect(&sel->start_drag);
    listener_disconnect(&sel->drag_drop);
    listener_disconnect(&sel->drag_destroy);
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

void selection_modifiers_changed(selection *sel, uint32_t modifiers)
{
    struct wlr_drag *drag = sel->seat->wlr_seat->drag;
    if (drag == NULL || drag->source == NULL) {
        return;
    }
    int32_t actions = drag_actions_for_modifiers(sel->offered_actions, modifiers);
    if (drag->source->actions != actions) {
        drag->source->actions = actions;
        refresh_offers(drag);
    }
}

static void handle_request_start_drag(struct wl_listener *listener, void *data)
{
    selection *sel = wl_container_of(listener, sel, request_start_drag);
    const struct wlr_seat_request_start_drag_event *event = data;
    struct wlr_seat *wlr_seat = sel->seat->wlr_seat;
    if (wlr_seat_validate_pointer_grab_serial(wlr_seat, event->origin, event->serial)) {
        offer_actions(sel, event->drag, wlr_seat->pointer_state.grab_button);
        wlr_seat_start_pointer_drag(wlr_seat, event->drag, event->serial);
        return;
    }
    struct wlr_touch_point *point;
    if (wlr_seat_validate_touch_grab_serial(wlr_seat, event->origin, event->serial, &point)) {
        offer_actions(sel, event->drag, 0);
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
    listener_disconnect(&sel->drag_drop);
    listener_disconnect(&sel->drag_destroy);
    listener_connect(&sel->drag_drop, &drag->events.drop, handle_drag_drop);
    listener_connect(&sel->drag_destroy, &drag->events.destroy, handle_drag_destroy);
    if (drag->icon != NULL) {
        wlr_scene_drag_icon_create(drag_tree(sel), drag->icon);
    }
    selection_move_icons(sel);
}

static void handle_drag_drop(struct wl_listener *listener, void *data)
{
    (void) listener;
    const struct wlr_drag_drop_event *event = data;
    keep_target_after_drop(event->drag);
}

static void handle_drag_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    selection *sel = wl_container_of(listener, sel, drag_destroy);
    listener_disconnect(&sel->drag_drop);
    listener_disconnect(&sel->drag_destroy);
}

static void offer_actions(selection *sel, struct wlr_drag *drag, uint32_t button)
{
    if (drag->source == NULL) {
        return;
    }
    drag_start start = {
        .source_actions = drag->source->actions,
        .button = button,
        .alt_held = (seat_modifiers(sel->seat) & WM_MOD_ALT) != 0,
    };
    sel->offered_actions = drag_actions_offered(&start);
    drag->source->actions =
        drag_actions_for_modifiers(sel->offered_actions, seat_modifiers(sel->seat));
}

static void keep_target_after_drop(struct wlr_drag *drag)
{
    bool button_still_held = drag->grab_type == WLR_DRAG_GRAB_KEYBOARD_POINTER &&
                             drag->seat->pointer_state.button_count > 0;
    if (drag->focus_client == NULL || button_still_held) {
        return;
    }
    wl_list_remove(&drag->WLR_PRIVATE.seat_client_destroy.link);
    wl_list_init(&drag->WLR_PRIVATE.seat_client_destroy.link);
    drag->focus_client = NULL;
}

static void refresh_offers(const struct wlr_drag *drag)
{
    struct wlr_data_offer *offer;
    wl_list_for_each(offer, &drag->seat->drag_offers, link) {
        if (offer->source == drag->source && offer->type == WLR_DATA_OFFER_DRAG) {
            refresh_offer(offer);
        }
    }
}

static void refresh_offer(struct wlr_data_offer *offer)
{
    int version = wl_resource_get_version(offer->resource);
    if (version >= WL_DATA_OFFER_SOURCE_ACTIONS_SINCE_VERSION && offer->source->actions >= 0) {
        wl_data_offer_send_source_actions(offer->resource, (uint32_t) offer->source->actions);
    }
    drag_choice choice = choice_for(offer);
    uint32_t action = drag_actions_choose(&choice);
    if (offer->source->current_dnd_action == action) {
        return;
    }
    offer->source->current_dnd_action = action;
    if (offer->in_ask) {
        return;
    }
    wlr_data_source_dnd_action(offer->source, action);
    if (choice.offer_sets_actions) {
        wl_data_offer_send_action(offer->resource, action);
    }
}

static drag_choice choice_for(const struct wlr_data_offer *offer)
{
    int version = wl_resource_get_version(offer->resource);
    return (drag_choice){
        .offer_sets_actions = version >= WL_DATA_OFFER_ACTION_SINCE_VERSION,
        .offer_actions = offer->actions,
        .preferred_action = offer->preferred_action,
        .source_actions = offer->source->actions,
    };
}

static struct wlr_scene_tree *drag_tree(const selection *sel)
{
    return scene_layers_tree(sel->seat->server->layers, SCENE_LAYER_DRAG);
}
