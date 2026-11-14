#include "cycle.h"

#include "desktop_state.h"
#include "keybindings.h"

typedef enum cycle_move {
    CYCLE_MOVE_NONE,
    CYCLE_MOVE_DIRECTION,
    CYCLE_MOVE_NEXT,
    CYCLE_MOVE_PREVIOUS,
} cycle_move;

static void move_by_key(wm_desktop *desktop, const wm_key_event *event);
static cycle_move move_for_key(const wm_desktop *desktop, const wm_key_event *event);
static bool is_reverse_key(const wm_desktop *desktop, const wm_key_event *event);
static void move_selection(wm_desktop *desktop, move_direction direction);
static void select_next(wm_desktop *desktop);
static void select_previous(wm_desktop *desktop);

wm_dispatch cycle_key(wm_desktop *desktop, const wm_key_event *event)
{
    if (keybindings_is_key_of(&desktop->keys, WM_ACTION_CANCEL, event->keysym)) {
        cycle_abort(desktop);
        return WM_CONSUMED;
    }
    if (keysym_is_return(event->keysym)) {
        cycle_end(desktop);
        return WM_CONSUMED;
    }
    move_by_key(desktop, event);
    if (!keysym_is_modifier(event->keysym) && !(event->modifiers & desktop->cycle.modifiers)) {
        cycle_end(desktop);
    }
    return WM_CONSUMED;
}

static void move_by_key(wm_desktop *desktop, const wm_key_event *event)
{
    switch (move_for_key(desktop, event)) {
    case CYCLE_MOVE_DIRECTION:
        move_selection(desktop, keybindings_direction_of(&desktop->keys, event->keysym));
        break;
    case CYCLE_MOVE_NEXT:
        select_next(desktop);
        break;
    case CYCLE_MOVE_PREVIOUS:
        select_previous(desktop);
        break;
    case CYCLE_MOVE_NONE:
        break;
    }
}

static cycle_move move_for_key(const wm_desktop *desktop, const wm_key_event *event)
{
    if (keybindings_direction_of(&desktop->keys, event->keysym) != DIRECTION_NONE) {
        return CYCLE_MOVE_DIRECTION;
    }
    wm_action action = keybindings_action_for(&desktop->keys, event);
    if (action == WM_ACTION_CYCLE_REVERSE_WINDOWS || is_reverse_key(desktop, event)) {
        return CYCLE_MOVE_PREVIOUS;
    }
    return action == WM_ACTION_CYCLE_WINDOWS ? CYCLE_MOVE_NEXT : CYCLE_MOVE_NONE;
}

bool cycle_key_repeats(const wm_desktop *desktop, const wm_key_event *event)
{
    return move_for_key(desktop, event) != CYCLE_MOVE_NONE;
}

wm_dispatch cycle_key_release(wm_desktop *desktop, const wm_key_event *event)
{
    if (keysym_is_modifier(event->keysym) && !(event->modifiers & desktop->cycle.modifiers)) {
        cycle_end(desktop);
    }
    return WM_CONSUMED;
}

void cycle_modifiers_changed(wm_desktop *desktop, uint32_t modifiers)
{
    if (!(modifiers & desktop->cycle.modifiers)) {
        cycle_end(desktop);
    }
}

wm_dispatch cycle_motion(wm_desktop *desktop, const wm_pointer_event *event)
{
    cycle_hover(desktop, event->target.kind == WM_TARGET_CYCLE ? event->target.item : -1);
    return WM_CONSUMED;
}

wm_dispatch cycle_button(wm_desktop *desktop, const wm_button_event *event)
{
    if (!event->pressed) {
        return WM_CONSUMED;
    }
    if (event->pointer.target.kind != WM_TARGET_CYCLE) {
        cycle_abort(desktop);
        return WM_CONSUMED;
    }
    if (event->button == WM_MOUSE_LEFT) {
        int item = event->pointer.target.item;
        if (item >= 0) {
            cycle_select(desktop, (size_t) item);
        }
        cycle_end(desktop);
    }
    return WM_CONSUMED;
}

wm_dispatch cycle_axis(wm_desktop *desktop, const wm_axis_event *event)
{
    if (event->pointer.target.kind != WM_TARGET_CYCLE || event->axis != WM_AXIS_VERTICAL) {
        return WM_CONSUMED;
    }
    if (event->steps < 0) {
        select_previous(desktop);
    } else if (event->steps > 0) {
        select_next(desktop);
    }
    return WM_CONSUMED;
}

static bool is_reverse_key(const wm_desktop *desktop, const wm_key_event *event)
{
    if (!(event->modifiers & WM_MOD_SHIFT)) {
        return false;
    }
    uint32_t keysym = event->keysym;
    wm_key_event plain = {keysym, event->modifiers & ~(uint32_t) WM_MOD_SHIFT, event->time_ms};
    if (keybindings_action_for(&desktop->keys, &plain) == WM_ACTION_CYCLE_WINDOWS) {
        return true;
    }
    plain.keysym = WM_KEYSYM_TAB;
    return keysym == WM_KEYSYM_ISO_LEFT_TAB
           && keybindings_action_for(&desktop->keys, &plain) == WM_ACTION_CYCLE_WINDOWS;
}

static void move_selection(wm_desktop *desktop, move_direction direction)
{
    cycle_session *cycle = &desktop->cycle;
    grid_selection selection = {cycle->count, cycle->selected};
    wm_point move = direction_offset(direction);
    cycle_select(desktop, tabwin_select_delta(&cycle->grid, selection, move));
}

static void select_next(wm_desktop *desktop)
{
    cycle_session *cycle = &desktop->cycle;
    cycle_select(desktop, (cycle->selected + 1) % cycle->count);
}

static void select_previous(wm_desktop *desktop)
{
    cycle_session *cycle = &desktop->cycle;
    cycle_select(desktop, (cycle->selected + cycle->count - 1) % cycle->count);
}
