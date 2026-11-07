#include <wlr/types/wlr_cursor_shape_v1.h>
#include <wlr/types/wlr_idle_inhibit_v1.h>
#include <wlr/types/wlr_idle_notify_v1.h>
#include <wlr/types/wlr_input_method_v2.h>
#include <wlr/types/wlr_keyboard_shortcuts_inhibit_v1.h>
#include <wlr/types/wlr_pointer_constraints_v1.h>
#include <wlr/types/wlr_pointer_gestures_v1.h>
#include <wlr/types/wlr_relative_pointer_v1.h>
#include <wlr/types/wlr_text_input_v3.h>
#include <wlr/types/wlr_virtual_keyboard_v1.h>
#include <wlr/types/wlr_virtual_pointer_v1.h>
#include <wlr/util/log.h>

#include "protocols.h"

enum {
    CURSOR_SHAPE_VERSION = 1,
};

bool protocols_create_input(compositor *server)
{
    struct wl_display *display = server->display;
    protocol_globals *globals = &server->globals;
    globals->relative_pointer = wlr_relative_pointer_manager_v1_create(display);
    globals->gestures = wlr_pointer_gestures_v1_create(display);
    globals->constraints = wlr_pointer_constraints_v1_create(display);
    globals->shortcuts_inhibit = wlr_keyboard_shortcuts_inhibit_v1_create(display);
    globals->cursor_shape = wlr_cursor_shape_manager_v1_create(display, CURSOR_SHAPE_VERSION);
    globals->virtual_keyboard = wlr_virtual_keyboard_manager_v1_create(display);
    globals->virtual_pointer = wlr_virtual_pointer_manager_v1_create(display);
    globals->idle_notifier = wlr_idle_notifier_v1_create(display);
    globals->idle_inhibit = wlr_idle_inhibit_v1_create(display);
    wlr_text_input_manager_v3_create(display);
    wlr_input_method_manager_v2_create(display);
    if (globals->gestures == NULL) {
        wlr_log(WLR_ERROR, "cannot create the pointer gestures global");
        return false;
    }
    return true;
}
