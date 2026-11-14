#include <stdlib.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_primary_selection.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>

#include "drag_actions.h"
#include "util/clock.h"
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
    struct wl_listener drag_focus;
    int32_t offered_actions;
    struct wl_event_source *drag_cursor_tick;
    int64_t drag_focus_ms;
};

enum {
    DRAG_CURSOR_TICK_MS = 50,
};

static void handle_request_set_selection(struct wl_listener *listener, void *data);
static void handle_request_set_primary(struct wl_listener *listener, void *data);
static void handle_request_start_drag(struct wl_listener *listener, void *data);
static void handle_start_drag(struct wl_listener *listener, void *data);
static void handle_drag_drop(struct wl_listener *listener, void *data);
static void handle_drag_destroy(struct wl_listener *listener, void *data);
static void handle_drag_focus(struct wl_listener *listener, void *data);
static void offer_actions(selection *selection, struct wlr_drag *drag, uint32_t button);
static void keep_target_after_drop(struct wlr_drag *drag);
static void refresh_offers(const struct wlr_drag *drag);
static void refresh_offer(struct wlr_data_offer *offer);
static drag_choice choice_for(const struct wlr_data_offer *offer);
static struct wlr_scene_tree *drag_tree(const selection *selection);
static layout_point drag_position(const selection *selection);
static void stop_drag_cursor(selection *selection);
static void show_drag_cursor(selection *selection);
static int handle_drag_cursor_tick(void *data);

selection *selection_create(seat *s)
{
    selection *sel = calloc(1, sizeof(*sel));
    if (sel == NULL) {
        return NULL;
    }
    sel->seat = s;
    sel->drag_cursor_tick = wl_event_loop_add_timer(s->server->loop, handle_drag_cursor_tick, sel);
    if (sel->drag_cursor_tick == NULL) {
        free(sel);
        return NULL;
    }
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
    wl_list_init(&sel->drag_focus.link);
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
    listener_disconnect(&sel->drag_focus);
    wl_event_source_remove(sel->drag_cursor_tick);
    free(sel);
}

void selection_drag_moved(selection *sel)
{
    show_drag_cursor(sel);
    struct wlr_scene_tree *tree = drag_tree(sel);
    if (wl_list_empty(&tree->children)) {
        return;
    }
    layout_point position = drag_position(sel);
    wlr_scene_node_set_position(&tree->node, (int) position.x, (int) position.y);
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
        show_drag_cursor(sel);
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
    listener_disconnect(&sel->drag_focus);
    listener_connect(&sel->drag_drop, &drag->events.drop, handle_drag_drop);
    listener_connect(&sel->drag_destroy, &drag->events.destroy, handle_drag_destroy);
    listener_connect(&sel->drag_focus, &drag->events.focus, handle_drag_focus);
    if (drag->icon != NULL) {
        wlr_scene_drag_icon_create(drag_tree(sel), drag->icon);
    }
    cursor_image_drag_started(sel->seat->image);
    sel->drag_focus_ms = clock_now_ms();
    if (drag->grab_type == WLR_DRAG_GRAB_KEYBOARD_POINTER) {
        wl_event_source_timer_update(sel->drag_cursor_tick, DRAG_CURSOR_TICK_MS);
    }
    selection_drag_moved(sel);
}

static void handle_drag_drop(struct wl_listener *listener, void *data)
{
    selection *sel = wl_container_of(listener, sel, drag_drop);
    const struct wlr_drag_drop_event *event = data;
    stop_drag_cursor(sel);
    keep_target_after_drop(event->drag);
}

static void handle_drag_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    selection *sel = wl_container_of(listener, sel, drag_destroy);
    listener_disconnect(&sel->drag_drop);
    listener_disconnect(&sel->drag_destroy);
    stop_drag_cursor(sel);
    pointer_input_forget_hit(sel->seat->pointer);
    seat_repick_pointer(sel->seat);
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

static void handle_drag_focus(struct wl_listener *listener, void *data)
{
    (void) data;
    selection *sel = wl_container_of(listener, sel, drag_focus);
    sel->drag_focus_ms = clock_now_ms();
    show_drag_cursor(sel);
}

static void stop_drag_cursor(selection *sel)
{
    listener_disconnect(&sel->drag_focus);
    wl_event_source_timer_update(sel->drag_cursor_tick, 0);
}

static void show_drag_cursor(selection *sel)
{
    const struct wlr_drag *drag = sel->seat->wlr_seat->drag;
    if (drag == NULL || drag->source == NULL
        || drag->grab_type != WLR_DRAG_GRAB_KEYBOARD_POINTER) {
        return;
    }
    bool accepted = drag->focus_client != NULL && drag->source->accepted;
    uint32_t action = accepted ? drag->source->current_dnd_action
                               : WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE;
    bool target_answering = clock_now_ms() - sel->drag_focus_ms < DRAG_CURSOR_TICK_MS;
    if (action == WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE && target_answering) {
        return;
    }
    cursor_image_show_drag_action(sel->seat->image, action);
}

static int handle_drag_cursor_tick(void *data)
{
    selection *sel = data;
    show_drag_cursor(sel);
    if (sel->seat->wlr_seat->drag != NULL) {
        wl_event_source_timer_update(sel->drag_cursor_tick, DRAG_CURSOR_TICK_MS);
    }
    return 0;
}

static struct wlr_scene_tree *drag_tree(const selection *sel)
{
    return scene_layers_tree(sel->seat->server->layers, SCENE_LAYER_DRAG);
}

static layout_point drag_position(const selection *sel)
{
    seat *s = sel->seat;
    layout_point position = {s->cursor->x, s->cursor->y};
    struct wlr_drag *drag = s->wlr_seat->drag;
    if (drag != NULL && drag->grab_type == WLR_DRAG_GRAB_KEYBOARD_TOUCH) {
        touch_input_position(s->touch, drag->touch_id, &position);
    }
    return position;
}
