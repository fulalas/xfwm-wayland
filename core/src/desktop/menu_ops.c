#include "menu.h"

#include "desktop_state.h"
#include "maximize.h"
#include "monitor_move.h"
#include "moveresize.h"
#include "states.h"
#include "visibility.h"
#include "window_ops.h"

typedef void (*menu_handler)(wm_desktop *desktop, wm_window *window);

typedef struct menu_op {
    wm_menu_label label;
    menu_handler run;
} menu_op;

static bool run_monitor_op(wm_desktop *desktop, wm_window *window, wm_menu_label label);

static const menu_op MENU_OPS[] = {
    {WM_MENU_MINIMIZE, window_ops_hide},
    {WM_MENU_MINIMIZE_OTHERS, visibility_minimize_others},
    {WM_MENU_SHOW, window_ops_show},
    {WM_MENU_MOVE, moveresize_begin_key_move},
    {WM_MENU_RESIZE, moveresize_begin_key_resize},
    {WM_MENU_ABOVE, states_toggle_above},
    {WM_MENU_NORMAL, states_set_normal_layer},
    {WM_MENU_BELOW, states_toggle_below},
    {WM_MENU_SHADE, states_toggle_shade},
    {WM_MENU_UNSHADE, states_toggle_shade},
    {WM_MENU_FULLSCREEN, states_toggle_fullscreen},
    {WM_MENU_UNFULLSCREEN, states_toggle_fullscreen},
    {WM_MENU_STICK, states_toggle_stick},
    {WM_MENU_CLOSE, window_ops_close},
};

static const struct {
    wm_menu_label label;
    monitor_direction direction;
} MONITOR_OPS[] = {
    {WM_MENU_MONITOR_LEFT, MONITOR_LEFT},
    {WM_MENU_MONITOR_RIGHT, MONITOR_RIGHT},
    {WM_MENU_MONITOR_UP, MONITOR_UP},
    {WM_MENU_MONITOR_DOWN, MONITOR_DOWN},
};

void menu_perform(wm_desktop *desktop, const menu_choice *choice)
{
    const wm_menu_item *item = &choice->item;
    if (item->label == WM_MENU_WORKSPACE_ENTRY) {
        visibility_set_workspace(desktop, choice->window, choice->index);
        return;
    }
    if (item->mark == WM_MENU_MARK_RADIO_ON) {
        return;
    }
    if (item->label == WM_MENU_MAXIMIZE || item->label == WM_MENU_UNMAXIMIZE) {
        maximize_toggle(desktop, choice->window, WM_STATE_MAXIMIZED);
        return;
    }
    if (run_monitor_op(desktop, choice->window, item->label)) {
        return;
    }
    for (size_t i = 0; i < sizeof(MENU_OPS) / sizeof(MENU_OPS[0]); i++) {
        if (MENU_OPS[i].label == item->label) {
            MENU_OPS[i].run(desktop, choice->window);
            return;
        }
    }
}

static bool run_monitor_op(wm_desktop *desktop, wm_window *window, wm_menu_label label)
{
    for (size_t i = 0; i < sizeof(MONITOR_OPS) / sizeof(MONITOR_OPS[0]); i++) {
        if (MONITOR_OPS[i].label == label) {
            monitor_move(desktop, window, MONITOR_OPS[i].direction);
            return true;
        }
    }
    return false;
}
