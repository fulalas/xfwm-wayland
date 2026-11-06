#ifndef DESKTOP_LAUNCHES_H
#define DESKTOP_LAUNCHES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "wm/desktop.h"

enum {
    LAUNCHES_MAX = 8,
};

typedef struct launch_entry {
    char id[WM_LAUNCH_ID_MAX];
    int workspace;
    uint32_t started_ms;
    uint32_t expires_ms;
    bool finished;
} launch_entry;

typedef struct launch_set {
    launch_entry entries[LAUNCHES_MAX];
    size_t count;
} launch_set;

bool launches_take(wm_desktop *desktop, const char *id, int *workspace);
bool launches_busy(const wm_desktop *desktop);
void launches_on_timer(wm_desktop *desktop);
void launches_workspace_removed(wm_desktop *desktop, int position);

#endif
