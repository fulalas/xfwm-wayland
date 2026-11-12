#include "actions.h"

#include "cycle.h"
#include "desktop_state.h"
#include "visibility.h"
#include "workspaces.h"

typedef void (*global_handler)(wm_desktop *desktop);

typedef struct global_action {
    wm_action action;
    global_handler run;
} global_action;

static void run_workspace_action(wm_desktop *desktop, wm_action action);
static void add_workspace(wm_desktop *desktop);
static void delete_workspace(wm_desktop *desktop);
static void add_adjacent_workspace(wm_desktop *desktop);
static void delete_active_workspace(wm_desktop *desktop);

static const global_action GLOBAL_ACTIONS[] = {
    {WM_ACTION_SWITCH_WINDOW, cycle_switch_window},
    {WM_ACTION_SWITCH_APPLICATION, cycle_switch_application},
    {WM_ACTION_ADD_WORKSPACE, add_workspace},
    {WM_ACTION_DEL_WORKSPACE, delete_workspace},
    {WM_ACTION_ADD_ADJACENT_WORKSPACE, add_adjacent_workspace},
    {WM_ACTION_DEL_ACTIVE_WORKSPACE, delete_active_workspace},
    {WM_ACTION_SHOW_DESKTOP, visibility_toggle_show_desktop},
};

static const struct {
    wm_action action;
    int step;
} STEP_ACTIONS[] = {
    {WM_ACTION_NEXT_WORKSPACE, 1},
    {WM_ACTION_PREV_WORKSPACE, -1},
};

static const struct {
    wm_action action;
    move_direction direction;
} DIRECTION_ACTIONS[] = {
    {WM_ACTION_LEFT_WORKSPACE, DIRECTION_LEFT},
    {WM_ACTION_RIGHT_WORKSPACE, DIRECTION_RIGHT},
    {WM_ACTION_UP_WORKSPACE, DIRECTION_UP},
    {WM_ACTION_DOWN_WORKSPACE, DIRECTION_DOWN},
};

void actions_run_global(wm_desktop *desktop, wm_action action)
{
    if (action >= WM_ACTION_WORKSPACE_1 && action <= WM_ACTION_WORKSPACE_12) {
        int target = (int) (action - WM_ACTION_WORKSPACE_1);
        if (target < desktop->workspaces.count) {
            workspaces_switch(desktop, target, NULL);
        }
        return;
    }
    for (size_t i = 0; i < sizeof(GLOBAL_ACTIONS) / sizeof(GLOBAL_ACTIONS[0]); i++) {
        if (GLOBAL_ACTIONS[i].action == action) {
            GLOBAL_ACTIONS[i].run(desktop);
            return;
        }
    }
    run_workspace_action(desktop, action);
}

static void run_workspace_action(wm_desktop *desktop, wm_action action)
{
    for (size_t i = 0; i < sizeof(STEP_ACTIONS) / sizeof(STEP_ACTIONS[0]); i++) {
        if (STEP_ACTIONS[i].action == action) {
            workspaces_switch(desktop, desktop->workspaces.current + STEP_ACTIONS[i].step, NULL);
        }
    }
    for (size_t i = 0; i < sizeof(DIRECTION_ACTIONS) / sizeof(DIRECTION_ACTIONS[0]); i++) {
        if (DIRECTION_ACTIONS[i].action == action) {
            workspaces_move(desktop, DIRECTION_ACTIONS[i].direction, NULL);
        }
    }
}

static void add_workspace(wm_desktop *desktop)
{
    workspaces_change_count(desktop, desktop->workspaces.count + 1);
}

static void delete_workspace(wm_desktop *desktop)
{
    workspaces_change_count(desktop, desktop->workspaces.count - 1);
}

static void add_adjacent_workspace(wm_desktop *desktop)
{
    workspaces_insert(desktop, desktop->workspaces.current + 1);
}

static void delete_active_workspace(wm_desktop *desktop)
{
    workspaces_delete(desktop, desktop->workspaces.current);
}
