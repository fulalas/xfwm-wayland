#include "cycle.h"

#include "desktop_state.h"
#include "keybindings.h"

static bool is_reverse_key(const wm_desktop *desktop, const wm_key_event *event);
static void move_selection(wm_desktop *desktop, int rows, int columns);
static void select_next(wm_desktop *desktop);
static void select_previous(wm_desktop *desktop);
static bool is_arrow(const wm_desktop *desktop, uint32_t keysym);
static void move_by_arrow(wm_desktop *desktop, uint32_t keysym);

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
    if (is_arrow(desktop, event->keysym)) {
        move_by_arrow(desktop, event->keysym);
    } else {
        wm_action action = keybindings_action_for(&desktop->keys, event);
        if (action == WM_ACTION_CYCLE_REVERSE_WINDOWS || is_reverse_key(desktop, event)) {
            select_previous(desktop);
        } else if (action == WM_ACTION_CYCLE_WINDOWS) {
            select_next(desktop);
        }
    }
    if (!keysym_is_modifier(event->keysym) && !(event->modifiers & desktop->cycle.modifiers)) {
        cycle_end(desktop);
    }
    return WM_CONSUMED;
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
    if (event->pointer.target.kind != WM_TARGET_CYCLE) {
        return WM_CONSUMED;
    }
    if (event->vertical_steps < 0) {
        select_previous(desktop);
    } else if (event->vertical_steps > 0) {
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

static void move_selection(wm_desktop *desktop, int rows, int columns)
{
    cycle_session *cycle = &desktop->cycle;
    grid_move move = {rows, columns};
    cycle_select(desktop, tabwin_select_delta(&cycle->grid, cycle->count, cycle->selected, move));
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

static bool is_arrow(const wm_desktop *desktop, uint32_t keysym)
{
    const keybindings *keys = &desktop->keys;
    return keybindings_is_key_of(keys, WM_ACTION_UP, keysym)
           || keybindings_is_key_of(keys, WM_ACTION_DOWN, keysym)
           || keybindings_is_key_of(keys, WM_ACTION_LEFT, keysym)
           || keybindings_is_key_of(keys, WM_ACTION_RIGHT, keysym);
}

static void move_by_arrow(wm_desktop *desktop, uint32_t keysym)
{
    const keybindings *keys = &desktop->keys;
    if (keybindings_is_key_of(keys, WM_ACTION_UP, keysym)) {
        move_selection(desktop, -1, 0);
    } else if (keybindings_is_key_of(keys, WM_ACTION_DOWN, keysym)) {
        move_selection(desktop, 1, 0);
    } else if (keybindings_is_key_of(keys, WM_ACTION_LEFT, keysym)) {
        move_selection(desktop, 0, -1);
    } else {
        move_selection(desktop, 0, 1);
    }
}
