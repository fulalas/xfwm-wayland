#include "launches.h"

#include <stdio.h>
#include <string.h>

#include "desktop_state.h"
#include "window_ops.h"

enum {
    LAUNCH_FEEDBACK_DELAY_MS = 150,
    LAUNCH_TIMEOUT_MS = 10000,
    LAUNCH_LATE_MS = 1000,
};

static int find(const launch_set *launches, const char *id);
static void remove_at(launch_set *launches, size_t index);
static void schedule(wm_desktop *desktop);
static uint32_t age_of(const wm_desktop *desktop, const launch_entry *entry);
static uint32_t time_left(const wm_desktop *desktop, const launch_entry *entry);

void wm_launch_started(wm_desktop *desktop, const char *id)
{
    launch_set *launches = &desktop->launches;
    if (id == NULL || *id == '\0' || find(launches, id) >= 0) {
        return;
    }
    if (launches->count == LAUNCHES_MAX) {
        remove_at(launches, 0);
    }
    launch_entry *entry = &launches->entries[launches->count++];
    uint32_t now = timers_now(desktop);
    *entry = (launch_entry) {.workspace = desktop->workspaces.current, .started_ms = now,
                             .expires_ms = now + LAUNCH_TIMEOUT_MS};
    snprintf(entry->id, sizeof(entry->id), "%s", id);
    schedule(desktop);
    present_flush(desktop);
}

void wm_launch_finished(wm_desktop *desktop, const char *id)
{
    int index = find(&desktop->launches, id);
    if (index < 0 || desktop->launches.entries[index].finished) {
        return;
    }
    launch_entry *entry = &desktop->launches.entries[index];
    entry->finished = true;
    entry->expires_ms = timers_now(desktop) + LAUNCH_LATE_MS;
    schedule(desktop);
    present_flush(desktop);
}

bool wm_window_launched(wm_desktop *desktop, wm_window *window, const char *id)
{
    int workspace;
    if (!launches_take(desktop, id, &workspace)) {
        return false;
    }
    bool fresh = timers_now(desktop) - window->mapped_ms < LAUNCH_LATE_MS;
    bool moved = fresh && window->parent == NULL && !window_is_sticky(window) &&
                 window->workspace != workspace;
    if (moved) {
        window_ops_send_to_workspace(desktop, window, workspace);
    }
    present_flush(desktop);
    return moved;
}

void launches_workspace_removed(wm_desktop *desktop, int position)
{
    launch_set *launches = &desktop->launches;
    for (size_t i = 0; i < launches->count; i++) {
        if (launches->entries[i].workspace > position) {
            launches->entries[i].workspace--;
        }
    }
}

bool launches_take(wm_desktop *desktop, const char *id, int *workspace)
{
    launch_set *launches = &desktop->launches;
    int index = find(launches, id);
    if (index < 0) {
        return false;
    }
    *workspace = launches->entries[index].workspace;
    remove_at(launches, (size_t) index);
    schedule(desktop);
    return true;
}

bool launches_busy(const wm_desktop *desktop)
{
    const launch_set *launches = &desktop->launches;
    for (size_t i = 0; i < launches->count; i++) {
        const launch_entry *entry = &launches->entries[i];
        if (!entry->finished && age_of(desktop, entry) >= LAUNCH_FEEDBACK_DELAY_MS) {
            return true;
        }
    }
    return false;
}

void launches_on_timer(wm_desktop *desktop)
{
    launch_set *launches = &desktop->launches;
    for (size_t i = launches->count; i > 0; i--) {
        if (time_left(desktop, &launches->entries[i - 1]) == 0) {
            remove_at(launches, i - 1);
        }
    }
    schedule(desktop);
}

static int find(const launch_set *launches, const char *id)
{
    for (size_t i = 0; id != NULL && i < launches->count; i++) {
        if (strcmp(launches->entries[i].id, id) == 0) {
            return (int) i;
        }
    }
    return -1;
}

static void remove_at(launch_set *launches, size_t index)
{
    memmove(&launches->entries[index], &launches->entries[index + 1],
            (launches->count - index - 1) * sizeof(launches->entries[0]));
    launches->count--;
}

static void schedule(wm_desktop *desktop)
{
    const launch_set *launches = &desktop->launches;
    timers_cancel_kind(desktop, TIMER_LAUNCH);
    uint32_t wait = UINT32_MAX;
    for (size_t i = 0; i < launches->count; i++) {
        const launch_entry *entry = &launches->entries[i];
        uint32_t age = age_of(desktop, entry);
        uint32_t next = time_left(desktop, entry);
        if (!entry->finished && age < LAUNCH_FEEDBACK_DELAY_MS) {
            next = LAUNCH_FEEDBACK_DELAY_MS - age;
        }
        wait = next < wait ? next : wait;
    }
    if (wait != UINT32_MAX) {
        timers_start(desktop, TIMER_LAUNCH, NULL, (int) wait);
    }
}

static uint32_t age_of(const wm_desktop *desktop, const launch_entry *entry)
{
    return timers_now(desktop) - entry->started_ms;
}

static uint32_t time_left(const wm_desktop *desktop, const launch_entry *entry)
{
    int32_t left = (int32_t) (entry->expires_ms - timers_now(desktop));
    return left > 0 ? (uint32_t) left : 0;
}
