#include "actions.h"

#include "cycle.h"
#include "desktop_state.h"
#include "fill.h"
#include "maximize.h"
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

static void run_tapped_modifier(wm_desktop *desktop, const wm_key_event *event);
static wm_dispatch press_app_shortcut(wm_desktop *desktop, const wm_key_event *event);
static void send_app_shortcut(wm_desktop *desktop, const wm_app_shortcut_event *event);
static bool is_grab_only(wm_action action);
static void run_action(wm_desktop *desktop, wm_action action);
static bool is_window_action(wm_action action);
static void run_focused(wm_desktop *desktop, wm_window *window, wm_action action);
static void run_unfocused(wm_desktop *desktop, wm_action action);
static void run_size_action(wm_desktop *desktop, wm_window *window, wm_action action);
static void run_place_action(wm_desktop *desktop, wm_window *window, wm_action action);
static void popup_menu(wm_desktop *desktop, wm_window *window);

static const window_action WINDOW_ACTIONS[] = {
    {WM_ACTION_CLOSE_WINDOW, window_ops_close},
    {WM_ACTION_HIDE_WINDOW, window_ops_hide},
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
};

static const struct {
    wm_action action;
    uint32_t mode;
} MAXIMIZE_ACTIONS[] = {
    {WM_ACTION_MAXIMIZE_WINDOW, WM_STATE_MAXIMIZED},
    {WM_ACTION_MAXIMIZE_VERT, WM_STATE_MAXIMIZED_VERT},
    {WM_ACTION_MAXIMIZE_HORIZ, WM_STATE_MAXIMIZED_HORZ},
};

static const struct {
    wm_action action;
    unsigned mode;
} FILL_ACTIONS[] = {
    {WM_ACTION_FILL_WINDOW, FILL_BOTH},
    {WM_ACTION_FILL_VERT, FILL_VERTICAL},
    {WM_ACTION_FILL_HORIZ, FILL_HORIZONTAL},
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

static const struct {
    wm_action action;
    int step;
} CARRY_STEP_ACTIONS[] = {
    {WM_ACTION_MOVE_NEXT_WORKSPACE, 1},
    {WM_ACTION_MOVE_PREV_WORKSPACE, -1},
};

static const struct {
    wm_action action;
    move_direction direction;
} CARRY_DIRECTION_ACTIONS[] = {
    {WM_ACTION_MOVE_LEFT_WORKSPACE, DIRECTION_LEFT},
    {WM_ACTION_MOVE_RIGHT_WORKSPACE, DIRECTION_RIGHT},
    {WM_ACTION_MOVE_UP_WORKSPACE, DIRECTION_UP},
    {WM_ACTION_MOVE_DOWN_WORKSPACE, DIRECTION_DOWN},
};

wm_dispatch actions_handle_key(wm_desktop *desktop, const wm_key_event *event)
{
    const char *command = keybindings_command_for(&desktop->keys, event);
    if (command != NULL) {
        if (keysym_is_held_modifier(event->keysym)) {
            desktop->tapped_modifier = *event;
            return WM_FORWARD;
        }
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
    run_action(desktop, action);
    return WM_CONSUMED;
}

void actions_cancel_tap(wm_desktop *desktop)
{
    desktop->tapped_modifier = (wm_key_event) {0};
}

wm_dispatch actions_release_key(wm_desktop *desktop, const wm_key_event *event)
{
    run_tapped_modifier(desktop, event);
    uint32_t id = desktop->held_app_shortcut;
    if (id == 0 || desktop->held_app_shortcut_keysym != event->keysym) {
        return WM_FORWARD;
    }
    desktop->held_app_shortcut = 0;
    send_app_shortcut(desktop, &(wm_app_shortcut_event) {id, WM_SHORTCUT_RELEASED, event->time_ms});
    return WM_CONSUMED;
}

static void run_tapped_modifier(wm_desktop *desktop, const wm_key_event *event)
{
    wm_key_event tapped = desktop->tapped_modifier;
    if (tapped.keysym != event->keysym) {
        return;
    }
    actions_cancel_tap(desktop);
    const char *command = keybindings_command_for(&desktop->keys, &tapped);
    if (command != NULL) {
        desktop->ports.commands.spawn(desktop->ports.commands.self, command);
    }
}

static wm_dispatch press_app_shortcut(wm_desktop *desktop, const wm_key_event *event)
{
    const wm_app_shortcut *shortcut = keybindings_app_shortcut_for(&desktop->keys, event);
    if (shortcut == NULL) {
        return WM_FORWARD;
    }
    if (desktop->held_app_shortcut != 0) {
        send_app_shortcut(desktop, &(wm_app_shortcut_event) {desktop->held_app_shortcut,
                                                             WM_SHORTCUT_RELEASED, event->time_ms});
    }
    desktop->held_app_shortcut = shortcut->id;
    desktop->held_app_shortcut_keysym = event->keysym;
    send_app_shortcut(desktop, &(wm_app_shortcut_event) {shortcut->id, WM_SHORTCUT_PRESSED,
                                                         event->time_ms});
    return WM_CONSUMED;
}

static bool is_grab_only(wm_action action)
{
    return action == WM_ACTION_CANCEL || action == WM_ACTION_UP || action == WM_ACTION_DOWN
           || action == WM_ACTION_LEFT || action == WM_ACTION_RIGHT;
}

static void run_action(wm_desktop *desktop, wm_action action)
{
    wm_window *focused = desktop->focus.focused;
    if (is_window_action(action) && focused != NULL) {
        run_focused(desktop, focused, action);
    } else if (is_window_action(action)) {
        run_unfocused(desktop, action);
    } else {
        actions_run_global(desktop, action);
    }
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
    run_size_action(desktop, window, action);
    run_place_action(desktop, window, action);
}

static void run_unfocused(wm_desktop *desktop, wm_action action)
{
    if (action == WM_ACTION_CYCLE_WINDOWS) {
        cycle_start(desktop, action);
    } else if (action == WM_ACTION_CLOSE_WINDOW) {
        desktop->ports.commands.logout(desktop->ports.commands.self);
    }
}

static void run_size_action(wm_desktop *desktop, wm_window *window, wm_action action)
{
    for (size_t i = 0; i < sizeof(MAXIMIZE_ACTIONS) / sizeof(MAXIMIZE_ACTIONS[0]); i++) {
        if (MAXIMIZE_ACTIONS[i].action == action) {
            maximize_toggle(desktop, window, MAXIMIZE_ACTIONS[i].mode);
        }
    }
    for (size_t i = 0; i < sizeof(FILL_ACTIONS) / sizeof(FILL_ACTIONS[0]); i++) {
        if (FILL_ACTIONS[i].action == action) {
            fill_window(desktop, window, FILL_ACTIONS[i].mode);
        }
    }
    for (size_t i = 0; i < sizeof(TILE_ACTIONS) / sizeof(TILE_ACTIONS[0]); i++) {
        if (TILE_ACTIONS[i].action == action) {
            tile_toggle(desktop, window, TILE_ACTIONS[i].tile);
        }
    }
}

static void run_place_action(wm_desktop *desktop, wm_window *window, wm_action action)
{
    for (size_t i = 0; i < sizeof(MONITOR_ACTIONS) / sizeof(MONITOR_ACTIONS[0]); i++) {
        if (MONITOR_ACTIONS[i].action == action) {
            monitor_move(desktop, window, MONITOR_ACTIONS[i].direction);
        }
    }
    for (size_t i = 0; i < sizeof(CARRY_STEP_ACTIONS) / sizeof(CARRY_STEP_ACTIONS[0]); i++) {
        if (CARRY_STEP_ACTIONS[i].action == action) {
            int target = desktop->workspaces.current + CARRY_STEP_ACTIONS[i].step;
            workspaces_switch(desktop, target, window);
        }
    }
    for (size_t i = 0; i < sizeof(CARRY_DIRECTION_ACTIONS) / sizeof(CARRY_DIRECTION_ACTIONS[0]);
         i++) {
        if (CARRY_DIRECTION_ACTIONS[i].action == action) {
            workspaces_move(desktop, CARRY_DIRECTION_ACTIONS[i].direction, window);
        }
    }
}

static void popup_menu(wm_desktop *desktop, wm_window *window)
{
    menu_show(desktop, window, (wm_point) {window->client.x, window->client.y});
    menu_show_mnemonics(desktop);
}

static void send_app_shortcut(wm_desktop *desktop, const wm_app_shortcut_event *event)
{
    wm_commands *commands = &desktop->ports.commands;
    commands->app_shortcut(commands->self, event);
}
