#include "menu.h"

#include "desktop_state.h"
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

static void maximize(wm_desktop *desktop, wm_window *window);
static void monitor_left(wm_desktop *desktop, wm_window *window);
static void monitor_right(wm_desktop *desktop, wm_window *window);
static void monitor_up(wm_desktop *desktop, wm_window *window);
static void monitor_down(wm_desktop *desktop, wm_window *window);

static const menu_op MENU_OPS[] = {
    {WM_MENU_MAXIMIZE, maximize},
    {WM_MENU_UNMAXIMIZE, maximize},
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
    {WM_MENU_MONITOR_LEFT, monitor_left},
    {WM_MENU_MONITOR_RIGHT, monitor_right},
    {WM_MENU_MONITOR_UP, monitor_up},
    {WM_MENU_MONITOR_DOWN, monitor_down},
    {WM_MENU_CLOSE, window_ops_close},
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
    for (size_t i = 0; i < sizeof(MENU_OPS) / sizeof(MENU_OPS[0]); i++) {
        if (MENU_OPS[i].label == item->label) {
            MENU_OPS[i].run(desktop, choice->window);
            return;
        }
    }
}

static void maximize(wm_desktop *desktop, wm_window *window)
{
    window_ops_maximize(desktop, window, WM_STATE_MAXIMIZED);
}

static void monitor_left(wm_desktop *desktop, wm_window *window)
{
    monitor_move(desktop, window, MONITOR_LEFT);
}

static void monitor_right(wm_desktop *desktop, wm_window *window)
{
    monitor_move(desktop, window, MONITOR_RIGHT);
}

static void monitor_up(wm_desktop *desktop, wm_window *window)
{
    monitor_move(desktop, window, MONITOR_UP);
}

static void monitor_down(wm_desktop *desktop, wm_window *window)
{
    monitor_move(desktop, window, MONITOR_DOWN);
}
