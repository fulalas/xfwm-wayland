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

static void next_workspace(wm_desktop *desktop);
static void previous_workspace(wm_desktop *desktop);
static void left_workspace(wm_desktop *desktop);
static void right_workspace(wm_desktop *desktop);
static void add_workspace(wm_desktop *desktop);
static void delete_workspace(wm_desktop *desktop);
static void add_adjacent_workspace(wm_desktop *desktop);
static void delete_active_workspace(wm_desktop *desktop);
static void ignore(wm_desktop *desktop);

static const global_action GLOBAL_ACTIONS[] = {
    {WM_ACTION_SWITCH_WINDOW, cycle_switch_window},
    {WM_ACTION_SWITCH_APPLICATION, cycle_switch_application},
    {WM_ACTION_NEXT_WORKSPACE, next_workspace},
    {WM_ACTION_PREV_WORKSPACE, previous_workspace},
    {WM_ACTION_LEFT_WORKSPACE, left_workspace},
    {WM_ACTION_RIGHT_WORKSPACE, right_workspace},
    {WM_ACTION_UP_WORKSPACE, ignore},
    {WM_ACTION_DOWN_WORKSPACE, ignore},
    {WM_ACTION_ADD_WORKSPACE, add_workspace},
    {WM_ACTION_DEL_WORKSPACE, delete_workspace},
    {WM_ACTION_ADD_ADJACENT_WORKSPACE, add_adjacent_workspace},
    {WM_ACTION_DEL_ACTIVE_WORKSPACE, delete_active_workspace},
    {WM_ACTION_SHOW_DESKTOP, visibility_toggle_show_desktop},
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
}

static void next_workspace(wm_desktop *desktop)
{
    workspaces_switch(desktop, desktop->workspaces.current + 1, NULL);
}

static void previous_workspace(wm_desktop *desktop)
{
    workspaces_switch(desktop, desktop->workspaces.current - 1, NULL);
}

static void left_workspace(wm_desktop *desktop)
{
    workspaces_move(desktop, -1, NULL);
}

static void right_workspace(wm_desktop *desktop)
{
    workspaces_move(desktop, 1, NULL);
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

static void ignore(wm_desktop *desktop)
{
    (void) desktop;
}
