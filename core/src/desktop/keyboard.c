#include "actions.h"
#include "cycle.h"
#include "desktop_state.h"
#include "focus.h"
#include "menu.h"
#include "moveresize.h"
#include "present.h"

static wm_dispatch dispatch_key(wm_desktop *desktop, const wm_key_event *event);

wm_dispatch wm_key_pressed(wm_desktop *desktop, const wm_key_event *event)
{
    focus_note_user_input(desktop, event->time_ms);
    wm_dispatch result = dispatch_key(desktop, event);
    present_flush(desktop);
    return result;
}

wm_dispatch wm_key_released(wm_desktop *desktop, const wm_key_event *event)
{
    wm_dispatch result = actions_release_key(desktop, event);
    bool grabbed = menu_is_active(desktop) || moveresize_is_active(desktop);
    if (result == WM_FORWARD && cycle_is_active(desktop)) {
        result = cycle_key_release(desktop, event);
    } else if (result == WM_FORWARD && grabbed) {
        result = WM_CONSUMED;
    }
    present_flush(desktop);
    return result;
}

void wm_modifiers_changed(wm_desktop *desktop, uint32_t modifiers)
{
    desktop->modifiers = modifiers;
    if (cycle_is_active(desktop)) {
        cycle_modifiers_changed(desktop, modifiers);
    }
    present_flush(desktop);
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
