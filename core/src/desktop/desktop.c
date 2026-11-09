#include <stdlib.h>
#include <string.h>

#include "desktop_state.h"
#include "maximize.h"
#include "monitor_move.h"
#include "monitors.h"
#include "placement.h"
#include "present.h"
#include "states.h"
#include "tile.h"
#include "view.h"
#include "visibility.h"
#include "workspaces.h"
#include "zoom.h"

static void apply_config(wm_desktop *desktop, const wm_config *previous);
static bool names_changed(const wm_config *a, const wm_config *b);
static void refit_window(wm_desktop *desktop, wm_window *window);
static void refit_all(wm_desktop *desktop);

wm_desktop *wm_desktop_create(const wm_ports *ports, const wm_config *config)
{
    wm_desktop *desktop = calloc(1, sizeof(*desktop));
    if (desktop == NULL) {
        return NULL;
    }
    desktop->ports = *ports;
    desktop->config = *config;
    desktop->workspaces.count = 1;
    desktop->workspaces.ids[0] = desktop->workspaces.next_id++;
    workspaces_set_count(desktop, config->workspaces.count);
    zoom_init(desktop);
    present_mark_workspaces(desktop);
    present_flush(desktop);
    return desktop;
}

void wm_desktop_destroy(wm_desktop *desktop)
{
    if (desktop == NULL) {
        return;
    }
    for (size_t i = 0; i < desktop->stack.count; i++) {
        window_destroy(desktop->stack.items[i]);
    }
    window_list_free(&desktop->stack);
    window_list_free(&desktop->mru);
    timers_free(&desktop->timers);
    keybindings_free(&desktop->keys);
    free(desktop);
}

void wm_desktop_set_appearance(wm_desktop *desktop, const wm_theme_metrics *theme,
                               const wm_config *config)
{
    wm_config previous = desktop->config;
    desktop->theme = *theme;
    desktop->config = *config;
    apply_config(desktop, &previous);
    present_flush(desktop);
}

void wm_desktop_set_keymap(wm_desktop *desktop, const wm_keymap *keymap)
{
    keybindings_copy(&desktop->keys, keymap);
}

void wm_desktop_outputs_changed(wm_desktop *desktop)
{
    refit_all(desktop);
    present_flush(desktop);
}

void wm_desktop_handle_timer(wm_desktop *desktop)
{
    timers_run_due(desktop);
    present_flush(desktop);
}

void wm_desktop_switch_workspace(wm_desktop *desktop, int workspace)
{
    workspaces_switch(desktop, workspace, NULL);
    present_flush(desktop);
}

void wm_desktop_add_workspace(wm_desktop *desktop)
{
    workspaces_change_count(desktop, desktop->workspaces.count + 1);
    present_flush(desktop);
}

void wm_desktop_set_workspace_count(wm_desktop *desktop, int count)
{
    workspaces_change_count(desktop, count);
    present_flush(desktop);
}

void wm_desktop_rename_workspaces(wm_desktop *desktop, const char *const *names, size_t count)
{
    const wm_workspace_config *config = &desktop->config.workspaces;
    const char *merged[WM_WORKSPACE_MAX];
    size_t merged_count = 0;
    bool changed = false;
    for (size_t i = 0; i < WM_WORKSPACE_MAX; i++) {
        merged[i] = i < count ? names[i] : config->names[i];
        if (*merged[i] != '\0') {
            merged_count = i + 1;
        }
        changed |= strncmp(config->names[i], merged[i], WM_WORKSPACE_NAME_MAX) != 0;
    }
    if (changed) {
        const wm_commands *commands = &desktop->ports.commands;
        commands->store_workspace_names(commands->self, merged, merged_count);
    }
}

void wm_desktop_set_workspace_layout(wm_desktop *desktop, int rows, int columns)
{
    int count = desktop->workspaces.count;
    if (rows <= 0 && columns > 0) {
        rows = (count + columns - 1) / columns;
    }
    if (rows > 0 && rows != desktop->config.workspaces.rows) {
        const wm_commands *commands = &desktop->ports.commands;
        commands->store_workspace_rows(commands->self, rows);
    }
}

void wm_desktop_remove_workspace(wm_desktop *desktop, int workspace)
{
    workspaces_delete(desktop, workspace);
    present_flush(desktop);
}

static void apply_config(wm_desktop *desktop, const wm_config *previous)
{
    workspaces_set_count(desktop, desktop->config.workspaces.count);
    if (names_changed(previous, &desktop->config) ||
        previous->workspaces.rows != desktop->config.workspaces.rows) {
        present_mark_workspaces(desktop);
    }
    if (!desktop->config.compositing.enabled) {
        zoom_reset(desktop);
    }
    view_invalidate_decor(desktop);
    refit_all(desktop);
}

static bool names_changed(const wm_config *a, const wm_config *b)
{
    for (int i = 0; i < WM_WORKSPACE_MAX; i++) {
        if (strncmp(a->workspaces.names[i], b->workspaces.names[i], WM_WORKSPACE_NAME_MAX) != 0) {
            return true;
        }
    }
    return false;
}

static void refit_window(wm_desktop *desktop, wm_window *window)
{
    if (window_is_fullscreen(window)) {
        states_update_fullscreen_size(desktop, window);
    } else if (window->tile != TILE_NONE) {
        tile_update_size(desktop, window);
    } else if (window_has_state(window, WM_STATE_MAXIMIZED)) {
        maximize_update_size(desktop, window);
    } else if (window_is_constrained(window)) {
        placement_constrain(desktop, window);
    }
    window_mark_dirty(window);
}

static void refit_all(wm_desktop *desktop)
{
    if (monitors_count(desktop) == 0) {
        return;
    }
    for (size_t i = 0; i < desktop->stack.count; i++) {
        wm_window *window = desktop->stack.items[i];
        monitor_move_follow_outputs(desktop, window);
        refit_window(desktop, window);
    }
    monitors_remember(desktop);
}
