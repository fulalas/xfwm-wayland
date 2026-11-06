#ifndef COMPOSITOR_EWMH_PAYLOAD_H
#define COMPOSITOR_EWMH_PAYLOAD_H

#include <stddef.h>
#include <stdint.h>

#include "wm/ports.h"
#include "wm/views.h"
#include "x11_atom_ids.h"

enum {
    EWMH_ALL_DESKTOPS = 0xFFFFFFFF,
    EWMH_EXTENT_COUNT = 4,
    EWMH_ACTION_MAX = 16,
    EWMH_NAMES_MAX = WM_WORKSPACE_MAX * WM_WORKSPACE_NAME_MAX,
};

typedef struct ewmh_frame_extents {
    uint32_t values[EWMH_EXTENT_COUNT];
} ewmh_frame_extents;

typedef struct ewmh_action_list {
    x11_atom_id atoms[EWMH_ACTION_MAX];
    size_t count;
} ewmh_action_list;

typedef struct ewmh_desktop_names {
    char bytes[EWMH_NAMES_MAX];
    size_t length;
} ewmh_desktop_names;

typedef struct ewmh_screen {
    wm_rect root;
    const wm_output_info *outputs;
    size_t count;
} ewmh_screen;

uint32_t ewmh_window_desktop(const wm_window_view *view);
ewmh_frame_extents ewmh_frame_extents_from(const wm_extents *extents);
ewmh_action_list ewmh_actions(uint32_t allowed_actions);
ewmh_desktop_names ewmh_names(const wm_workspaces_view *view);
size_t ewmh_split_names(const char *bytes, size_t length, const char **names, size_t max);
wm_rect ewmh_workarea(const ewmh_screen *screen);

#endif
