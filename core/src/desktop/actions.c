#include "actions.h"

#include "cycle.h"
#include "desktop_state.h"
#include "fill.h"
#include "menu.h"
#include "monitor_move.h"
#include "moveresize.h"
#include "stack.h"
#include "states.h"
#include "tile.h"
#include "window_ops.h"
#include "workspaces.h"

typedef void (*window_handler)(wm_desktop *desktop, wm_window *window);

typedef struct window_action {
    wm_action action;
    window_handler run;
} window_action;

static wm_dispatch press_app_shortcut(wm_desktop *desktop, const wm_key_event *event);
static bool is_grab_only(wm_action action);
static bool is_window_action(wm_action action);
static void run_focused(wm_desktop *desktop, wm_window *window, wm_action action);
static void run_unfocused(wm_desktop *desktop, wm_action action);
static void run_tile_action(wm_desktop *desktop, wm_window *window, wm_action action);
static void maximize_both(wm_desktop *desktop, wm_window *window);
static void maximize_vertically(wm_desktop *desktop, wm_window *window);
static void maximize_horizontally(wm_desktop *desktop, wm_window *window);
static void fill_both(wm_desktop *desktop, wm_window *window);
static void fill_vertically(wm_desktop *desktop, wm_window *window);
static void fill_horizontally(wm_desktop *desktop, wm_window *window);
static void popup_menu(wm_desktop *desktop, wm_window *window);
static void carry_to_next(wm_desktop *desktop, wm_window *window);
static void carry_to_previous(wm_desktop *desktop, wm_window *window);
static void carry_left(wm_desktop *desktop, wm_window *window);
static void carry_right(wm_desktop *desktop, wm_window *window);
static void carry_up(wm_desktop *desktop, wm_window *window);
static void carry_down(wm_desktop *desktop, wm_window *window);

static const window_action WINDOW_ACTIONS[] = {
    {WM_ACTION_CLOSE_WINDOW, window_ops_close},
    {WM_ACTION_HIDE_WINDOW, window_ops_hide},
    {WM_ACTION_MAXIMIZE_WINDOW, maximize_both},
    {WM_ACTION_MAXIMIZE_VERT, maximize_vertically},
    {WM_ACTION_MAXIMIZE_HORIZ, maximize_horizontally},
    {WM_ACTION_SHADE_WINDOW, states_toggle_shade},
    {WM_ACTION_STICK_WINDOW, window_ops_toggle_stick},
    {WM_ACTION_TOGGLE_ABOVE, states_toggle_above},
    {WM_ACTION_TOGGLE_FULLSCREEN, states_toggle_fullscreen},
    {WM_ACTION_MOVE, moveresize_begin_key_move},
    {WM_ACTION_RESIZE, moveresize_begin_key_resize},
    {WM_ACTION_RAISE_WINDOW, stack_raise},
    {WM_ACTION_LOWER_WINDOW, stack_lower},
    {WM_ACTION_RAISELOWER_WINDOW, window_ops_raise_or_lower},
    {WM_ACTION_POPUP_MENU, popup_menu},
    {WM_ACTION_FILL_WINDOW, fill_both},
    {WM_ACTION_FILL_VERT, fill_vertically},
    {WM_ACTION_FILL_HORIZ, fill_horizontally},
    {WM_ACTION_MOVE_NEXT_WORKSPACE, carry_to_next},
    {WM_ACTION_MOVE_PREV_WORKSPACE, carry_to_previous},
    {WM_ACTION_MOVE_LEFT_WORKSPACE, carry_left},
    {WM_ACTION_MOVE_RIGHT_WORKSPACE, carry_right},
    {WM_ACTION_MOVE_UP_WORKSPACE, carry_up},
    {WM_ACTION_MOVE_DOWN_WORKSPACE, carry_down},
};

static const struct {
    wm_action action;
    tile_mode tile;
} TILE_ACTIONS[] = {
    {WM_ACTION_TILE_DOWN, TILE_DOWN}, {WM_ACTION_TILE_LEFT, TILE_LEFT},
    {WM_ACTION_TILE_RIGHT, TILE_RIGHT}, {WM_ACTION_TILE_UP, TILE_UP},
    {WM_ACTION_TILE_DOWN_LEFT, TILE_DOWN_LEFT}, {WM_ACTION_TILE_DOWN_RIGHT, TILE_DOWN_RIGHT},
    {WM_ACTION_TILE_UP_LEFT, TILE_UP_LEFT}, {WM_ACTION_TILE_UP_RIGHT, TILE_UP_RIGHT},
};

static const struct {
    wm_action action;
    monitor_direction direction;
} MONITOR_ACTIONS[] = {
    {WM_ACTION_MOVE_TO_MONITOR_LEFT, MONITOR_LEFT},
    {WM_ACTION_MOVE_TO_MONITOR_RIGHT, MONITOR_RIGHT},
    {WM_ACTION_MOVE_TO_MONITOR_UP, MONITOR_UP},
    {WM_ACTION_MOVE_TO_MONITOR_DOWN, MONITOR_DOWN},
};

wm_dispatch actions_handle_key(wm_desktop *desktop, const wm_key_event *event)
{
    const char *command = keybindings_command_for(&desktop->keys, event);
    if (command != NULL) {
        desktop->ports.commands.spawn(desktop->ports.commands.self, command);
        return WM_CONSUMED;
    }
    wm_action action = keybindings_action_for(&desktop->keys, event);
    if (action == WM_ACTION_NONE) {
        return press_app_shortcut(desktop, event);
    }
    if (is_grab_only(action)) {
        return WM_FORWARD;
    }
    wm_window *focused = desktop->focus.focused;
    if (is_window_action(action) && focused != NULL) {
        run_focused(desktop, focused, action);
    } else if (is_window_action(action)) {
        run_unfocused(desktop, action);
    } else {
        actions_run_global(desktop, action);
    }
    return WM_CONSUMED;
}

wm_dispatch actions_release_key(wm_desktop *desktop, const wm_key_event *event)
{
    uint32_t id = desktop->held_app_shortcut;
    if (id == 0 || desktop->held_app_shortcut_keysym != event->keysym) {
        return WM_FORWARD;
    }
    desktop->held_app_shortcut = 0;
    wm_commands *commands = &desktop->ports.commands;
    commands->app_shortcut(commands->self, id, false, event->time_ms);
    return WM_CONSUMED;
}

static wm_dispatch press_app_shortcut(wm_desktop *desktop, const wm_key_event *event)
{
    const wm_app_shortcut *shortcut = keybindings_app_shortcut_for(&desktop->keys, event);
    if (shortcut == NULL) {
        return WM_FORWARD;
    }
    wm_commands *commands = &desktop->ports.commands;
    if (desktop->held_app_shortcut != 0) {
        commands->app_shortcut(commands->self, desktop->held_app_shortcut, false, event->time_ms);
    }
    desktop->held_app_shortcut = shortcut->id;
    desktop->held_app_shortcut_keysym = event->keysym;
    commands->app_shortcut(commands->self, shortcut->id, true, event->time_ms);
    return WM_CONSUMED;
}

static bool is_grab_only(wm_action action)
{
    return action == WM_ACTION_CANCEL || action == WM_ACTION_UP || action == WM_ACTION_DOWN
           || action == WM_ACTION_LEFT || action == WM_ACTION_RIGHT;
}

static bool is_window_action(wm_action action)
{
    switch (action) {
    case WM_ACTION_SWITCH_WINDOW:
    case WM_ACTION_SWITCH_APPLICATION:
    case WM_ACTION_NEXT_WORKSPACE:
    case WM_ACTION_PREV_WORKSPACE:
    case WM_ACTION_UP_WORKSPACE:
    case WM_ACTION_DOWN_WORKSPACE:
    case WM_ACTION_LEFT_WORKSPACE:
    case WM_ACTION_RIGHT_WORKSPACE:
    case WM_ACTION_ADD_WORKSPACE:
    case WM_ACTION_DEL_WORKSPACE:
    case WM_ACTION_ADD_ADJACENT_WORKSPACE:
    case WM_ACTION_DEL_ACTIVE_WORKSPACE:
    case WM_ACTION_SHOW_DESKTOP:
        return false;
    default:
        return action < WM_ACTION_WORKSPACE_1 || action > WM_ACTION_WORKSPACE_12;
    }
}

static void run_focused(wm_desktop *desktop, wm_window *window, wm_action action)
{
    if (action == WM_ACTION_CYCLE_WINDOWS || action == WM_ACTION_CYCLE_REVERSE_WINDOWS) {
        cycle_start(desktop, action);
        return;
    }
    if (action >= WM_ACTION_MOVE_WORKSPACE_1 && action <= WM_ACTION_MOVE_WORKSPACE_12) {
        window_ops_send_to_workspace(desktop, window, (int) (action - WM_ACTION_MOVE_WORKSPACE_1));
        return;
    }
    for (size_t i = 0; i < sizeof(WINDOW_ACTIONS) / sizeof(WINDOW_ACTIONS[0]); i++) {
        if (WINDOW_ACTIONS[i].action == action) {
            WINDOW_ACTIONS[i].run(desktop, window);
            return;
        }
    }
    run_tile_action(desktop, window, action);
}

static void run_unfocused(wm_desktop *desktop, wm_action action)
{
    if (action == WM_ACTION_CYCLE_WINDOWS) {
        cycle_start(desktop, action);
    } else if (action == WM_ACTION_CLOSE_WINDOW) {
        desktop->ports.commands.logout(desktop->ports.commands.self);
    }
}

static void run_tile_action(wm_desktop *desktop, wm_window *window, wm_action action)
{
    for (size_t i = 0; i < sizeof(TILE_ACTIONS) / sizeof(TILE_ACTIONS[0]); i++) {
        if (TILE_ACTIONS[i].action == action) {
            tile_toggle(desktop, window, TILE_ACTIONS[i].tile);
        }
    }
    for (size_t i = 0; i < sizeof(MONITOR_ACTIONS) / sizeof(MONITOR_ACTIONS[0]); i++) {
        if (MONITOR_ACTIONS[i].action == action) {
            monitor_move(desktop, window, MONITOR_ACTIONS[i].direction);
        }
    }
}

static void maximize_both(wm_desktop *desktop, wm_window *window)
{
    window_ops_maximize(desktop, window, WM_STATE_MAXIMIZED);
}

static void maximize_vertically(wm_desktop *desktop, wm_window *window)
{
    window_ops_maximize(desktop, window, WM_STATE_MAXIMIZED_VERT);
}

static void maximize_horizontally(wm_desktop *desktop, wm_window *window)
{
    window_ops_maximize(desktop, window, WM_STATE_MAXIMIZED_HORZ);
}

static void fill_both(wm_desktop *desktop, wm_window *window)
{
    fill_window(desktop, window, FILL_BOTH);
}

static void fill_vertically(wm_desktop *desktop, wm_window *window)
{
    fill_window(desktop, window, FILL_VERTICAL);
}

static void fill_horizontally(wm_desktop *desktop, wm_window *window)
{
    fill_window(desktop, window, FILL_HORIZONTAL);
}

static void popup_menu(wm_desktop *desktop, wm_window *window)
{
    menu_show(desktop, window, (wm_point) {window->client.x, window->client.y});
}

static void carry_to_next(wm_desktop *desktop, wm_window *window)
{
    workspaces_switch(desktop, desktop->workspaces.current + 1, window);
}

static void carry_to_previous(wm_desktop *desktop, wm_window *window)
{
    workspaces_switch(desktop, desktop->workspaces.current - 1, window);
}

static void carry_left(wm_desktop *desktop, wm_window *window)
{
    workspaces_move(desktop, 0, -1, window);
}

static void carry_right(wm_desktop *desktop, wm_window *window)
{
    workspaces_move(desktop, 0, 1, window);
}

static void carry_up(wm_desktop *desktop, wm_window *window)
{
    workspaces_move(desktop, -1, 0, window);
}

static void carry_down(wm_desktop *desktop, wm_window *window)
{
    workspaces_move(desktop, 1, 0, window);
}
