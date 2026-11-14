#include "actions.h"
#include "cycle.h"
#include "desktop_state.h"
#include "focus.h"
#include "menu.h"
#include "moveresize.h"
#include "present.h"

static wm_dispatch dispatch_key(wm_desktop *desktop, const wm_key_event *event);
static bool is_grabbed(const wm_desktop *desktop);

wm_dispatch wm_key_pressed(wm_desktop *desktop, const wm_key_event *event)
{
    focus_note_user_input(desktop, event->time_ms);
    actions_cancel_tap(desktop);
    wm_dispatch result = dispatch_key(desktop, event);
    present_flush(desktop);
    return result;
}

bool wm_key_repeats(const wm_desktop *desktop, const wm_key_event *event)
{
    if (menu_is_active(desktop)) {
        return menu_key_repeats(desktop, event);
    }
    if (cycle_is_active(desktop)) {
        return cycle_key_repeats(desktop, event);
    }
    if (moveresize_is_active(desktop)) {
        return moveresize_key_repeats(desktop, event);
    }
    return actions_key_repeats(desktop, event);
}

wm_dispatch wm_key_released(wm_desktop *desktop, const wm_key_event *event)
{
    wm_dispatch result = actions_release_key(desktop, event);
    if (result == WM_FORWARD && cycle_is_active(desktop)) {
        result = cycle_key_release(desktop, event);
    } else if (result == WM_FORWARD && is_grabbed(desktop)) {
        result = WM_CONSUMED;
    }
    present_flush(desktop);
    return result;
}

bool wm_key_is_bound(const wm_desktop *desktop, const wm_key_event *event)
{
    const keybindings *keys = &desktop->keys;
    return keybindings_command_for(keys, event) != NULL ||
           keybindings_action_for(keys, event) != WM_ACTION_NONE ||
           keybindings_app_shortcut_for(keys, event) != NULL;
}

void wm_modifiers_changed(wm_desktop *desktop, uint32_t modifiers)
{
    desktop->modifiers = modifiers;
    if (cycle_is_active(desktop)) {
        cycle_modifiers_changed(desktop, modifiers);
    }
    present_flush(desktop);
}

static bool is_grabbed(const wm_desktop *desktop)
{
    return menu_is_active(desktop) || cycle_is_active(desktop) || moveresize_is_active(desktop);
}

static wm_dispatch dispatch_key(wm_desktop *desktop, const wm_key_event *event)
{
    if (menu_is_active(desktop)) {
        return menu_key(desktop, event);
    }
    if (cycle_is_active(desktop)) {
        return cycle_key(desktop, event);
    }
    if (moveresize_is_active(desktop)) {
        return moveresize_key(desktop, event);
    }
    return actions_handle_key(desktop, event);
}
