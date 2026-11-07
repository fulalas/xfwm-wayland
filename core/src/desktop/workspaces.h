#ifndef DESKTOP_WORKSPACES_H
#define DESKTOP_WORKSPACES_H

#include <stdbool.h>

#include "direction.h"
#include "wm/desktop.h"

typedef struct workspace_state {
    int count;
    unsigned ids[WM_WORKSPACE_MAX];
    unsigned next_id;
    int current;
    int previous;
    bool show_desktop;
} workspace_state;

void workspaces_switch(wm_desktop *desktop, int workspace, wm_window *carried);
void workspaces_switch_without_focus(wm_desktop *desktop, int workspace, wm_window *carried);
void workspaces_move(wm_desktop *desktop, move_direction direction, wm_window *carried);
bool workspaces_move_changed(wm_desktop *desktop, move_direction direction, wm_window *carried);
void workspaces_set_count(wm_desktop *desktop, int count);
void workspaces_change_count(wm_desktop *desktop, int count);
void workspaces_insert(wm_desktop *desktop, int position);
void workspaces_delete(wm_desktop *desktop, int position);
wm_workspaces_view workspaces_view(const wm_desktop *desktop);

#endif
