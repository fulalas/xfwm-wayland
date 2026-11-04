#include "wm/keys.h"

#include <stdbool.h>
#include <string.h>

enum {
    NUMBERED_WORKSPACE_COUNT = WM_ACTION_WORKSPACE_12 - WM_ACTION_WORKSPACE_1 + 1,
    DECIMAL_BASE = 10,
    WORKSPACE_NUMBER_MAX_DIGITS = 2,
};

typedef struct action_name {
    const char *name;
    wm_action action;
} action_name;

typedef struct numbered_action {
    const char *prefix;
    wm_action first;
} numbered_action;

static const char key_suffix[] = "_key";

static const action_name action_names[] = {
    {"cancel_key", WM_ACTION_CANCEL},
    {"down_key", WM_ACTION_DOWN},
    {"left_key", WM_ACTION_LEFT},
    {"right_key", WM_ACTION_RIGHT},
    {"up_key", WM_ACTION_UP},
    {"add_workspace_key", WM_ACTION_ADD_WORKSPACE},
    {"add_adjacent_workspace_key", WM_ACTION_ADD_ADJACENT_WORKSPACE},
    {"close_window_key", WM_ACTION_CLOSE_WINDOW},
    {"cycle_windows_key", WM_ACTION_CYCLE_WINDOWS},
    {"cycle_reverse_windows_key", WM_ACTION_CYCLE_REVERSE_WINDOWS},
    {"del_workspace_key", WM_ACTION_DEL_WORKSPACE},
    {"del_active_workspace_key", WM_ACTION_DEL_ACTIVE_WORKSPACE},
    {"down_workspace_key", WM_ACTION_DOWN_WORKSPACE},
    {"fill_horiz_key", WM_ACTION_FILL_HORIZ},
    {"fill_vert_key", WM_ACTION_FILL_VERT},
    {"fill_window_key", WM_ACTION_FILL_WINDOW},
    {"hide_window_key", WM_ACTION_HIDE_WINDOW},
    {"left_workspace_key", WM_ACTION_LEFT_WORKSPACE},
    {"lower_window_key", WM_ACTION_LOWER_WINDOW},
    {"move_window_key", WM_ACTION_MOVE},
    {"maximize_horiz_key", WM_ACTION_MAXIMIZE_HORIZ},
    {"maximize_vert_key", WM_ACTION_MAXIMIZE_VERT},
    {"maximize_window_key", WM_ACTION_MAXIMIZE_WINDOW},
    {"move_window_to_monitor_down_key", WM_ACTION_MOVE_TO_MONITOR_DOWN},
    {"move_window_to_monitor_left_key", WM_ACTION_MOVE_TO_MONITOR_LEFT},
    {"move_window_to_monitor_right_key", WM_ACTION_MOVE_TO_MONITOR_RIGHT},
    {"move_window_to_monitor_up_key", WM_ACTION_MOVE_TO_MONITOR_UP},
    {"move_window_down_workspace_key", WM_ACTION_MOVE_DOWN_WORKSPACE},
    {"move_window_left_workspace_key", WM_ACTION_MOVE_LEFT_WORKSPACE},
    {"move_window_next_workspace_key", WM_ACTION_MOVE_NEXT_WORKSPACE},
    {"move_window_prev_workspace_key", WM_ACTION_MOVE_PREV_WORKSPACE},
    {"move_window_right_workspace_key", WM_ACTION_MOVE_RIGHT_WORKSPACE},
    {"move_window_up_workspace_key", WM_ACTION_MOVE_UP_WORKSPACE},
    {"move_window_down_key", WM_ACTION_MOVE_DOWN_WORKSPACE},
    {"move_window_left_key", WM_ACTION_MOVE_LEFT_WORKSPACE},
    {"move_window_right_key", WM_ACTION_MOVE_RIGHT_WORKSPACE},
    {"move_window_up_key", WM_ACTION_MOVE_UP_WORKSPACE},
    {"next_workspace_key", WM_ACTION_NEXT_WORKSPACE},
    {"popup_menu_key", WM_ACTION_POPUP_MENU},
    {"prev_workspace_key", WM_ACTION_PREV_WORKSPACE},
    {"raise_window_key", WM_ACTION_RAISE_WINDOW},
    {"raiselower_window_key", WM_ACTION_RAISELOWER_WINDOW},
    {"resize_window_key", WM_ACTION_RESIZE},
    {"right_workspace_key", WM_ACTION_RIGHT_WORKSPACE},
    {"shade_window_key", WM_ACTION_SHADE_WINDOW},
    {"show_desktop_key", WM_ACTION_SHOW_DESKTOP},
    {"stick_window_key", WM_ACTION_STICK_WINDOW},
    {"switch_application_key", WM_ACTION_SWITCH_APPLICATION},
    {"switch_window_key", WM_ACTION_SWITCH_WINDOW},
    {"tile_down_key", WM_ACTION_TILE_DOWN},
    {"tile_left_key", WM_ACTION_TILE_LEFT},
    {"tile_right_key", WM_ACTION_TILE_RIGHT},
    {"tile_up_key", WM_ACTION_TILE_UP},
    {"tile_down_left_key", WM_ACTION_TILE_DOWN_LEFT},
    {"tile_down_right_key", WM_ACTION_TILE_DOWN_RIGHT},
    {"tile_up_left_key", WM_ACTION_TILE_UP_LEFT},
    {"tile_up_right_key", WM_ACTION_TILE_UP_RIGHT},
    {"above_key", WM_ACTION_TOGGLE_ABOVE},
    {"fullscreen_key", WM_ACTION_TOGGLE_FULLSCREEN},
    {"up_workspace_key", WM_ACTION_UP_WORKSPACE},
};

static const numbered_action numbered_actions[] = {
    {"move_window_workspace_", WM_ACTION_MOVE_WORKSPACE_1},
    {"workspace_", WM_ACTION_WORKSPACE_1},
};

static wm_action named_action(const char *name);
static wm_action numbered_action_from_name(const char *name);
static int workspace_number(const char *text);
static bool is_digit(char character);

wm_action wm_action_from_name(const char *name)
{
    if (name == NULL) {
        return WM_ACTION_NONE;
    }
    wm_action action = named_action(name);
    if (action != WM_ACTION_NONE) {
        return action;
    }
    return numbered_action_from_name(name);
}

static wm_action named_action(const char *name)
{
    for (size_t i = 0; i < sizeof action_names / sizeof action_names[0]; i++) {
        if (strcmp(action_names[i].name, name) == 0) {
            return action_names[i].action;
        }
    }
    return WM_ACTION_NONE;
}

static wm_action numbered_action_from_name(const char *name)
{
    for (size_t i = 0; i < sizeof numbered_actions / sizeof numbered_actions[0]; i++) {
        size_t prefix_length = strlen(numbered_actions[i].prefix);
        if (strncmp(name, numbered_actions[i].prefix, prefix_length) != 0) {
            continue;
        }
        int number = workspace_number(name + prefix_length);
        if (number > 0) {
            return (wm_action) (numbered_actions[i].first + number - 1);
        }
    }
    return WM_ACTION_NONE;
}

static int workspace_number(const char *text)
{
    int number = 0;
    size_t digits = 0;

    if (text[0] == '0') {
        return 0;
    }
    while (digits < WORKSPACE_NUMBER_MAX_DIGITS && is_digit(text[digits])) {
        number = number * DECIMAL_BASE + (text[digits] - '0');
        digits++;
    }
    if (digits == 0 || number > NUMBERED_WORKSPACE_COUNT
        || strcmp(text + digits, key_suffix) != 0) {
        return 0;
    }
    return number;
}

static bool is_digit(char character)
{
    return character >= '0' && character <= '9';
}
