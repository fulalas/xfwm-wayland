#include <stdlib.h>
#include <string.h>

#include "desktop_state.h"
#include "maximize.h"
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
    for (size_t i = 0; i < desktop->stack.count; i++) {
        refit_window(desktop, desktop->stack.items[i]);
    }
}
