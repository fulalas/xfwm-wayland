#include "timers.h"

#include <stdlib.h>

#include "desktop_state.h"
#include "click.h"
#include "focus.h"
#include "launches.h"
#include "urgency.h"

enum {
    TIMERS_INITIAL_CAPACITY = 8,
    TIMERS_GROWTH_FACTOR = 2,
};

static bool is_due(uint32_t deadline_ms, uint32_t now_ms);
static bool find_due(const timer_set *timers, uint32_t now_ms, timer_entry *due);
static void remove_at(timer_set *timers, size_t index);
static bool grow(timer_set *timers);
static void dispatch(wm_desktop *desktop, const timer_entry *entry);
static bool earliest_deadline(const timer_set *timers, uint32_t *deadline_ms);

void timers_free(timer_set *timers)
{
    free(timers->entries);
    timers->entries = NULL;
    timers->count = 0;
    timers->capacity = 0;
}

void timers_start(wm_desktop *desktop, timer_kind kind, wm_window *window, int delay_ms)
{
    timer_set *timers = &desktop->timers;
    timers_cancel(desktop, kind, window);
    if (!grow(timers)) {
        return;
    }
    uint32_t delay = delay_ms > 0 ? (uint32_t) delay_ms : 0;
    timers->entries[timers->count++] = (timer_entry) {kind, window, timers_now(desktop) + delay};
}

void timers_cancel(wm_desktop *desktop, timer_kind kind, wm_window *window)
{
    timer_set *timers = &desktop->timers;
    for (size_t i = timers->count; i > 0; i--) {
        if (timers->entries[i - 1].kind == kind && timers->entries[i - 1].window == window) {
            remove_at(timers, i - 1);
        }
    }
}

void timers_cancel_kind(wm_desktop *desktop, timer_kind kind)
{
    timer_set *timers = &desktop->timers;
    for (size_t i = timers->count; i > 0; i--) {
        if (timers->entries[i - 1].kind == kind) {
            remove_at(timers, i - 1);
        }
    }
}

void timers_cancel_window(wm_desktop *desktop, const wm_window *window)
{
    timer_set *timers = &desktop->timers;
    for (size_t i = timers->count; i > 0; i--) {
        if (timers->entries[i - 1].window == window) {
            remove_at(timers, i - 1);
        }
    }
}

bool timers_is_running(const wm_desktop *desktop, timer_kind kind, const wm_window *window)
{
    const timer_set *timers = &desktop->timers;
    for (size_t i = 0; i < timers->count; i++) {
        if (timers->entries[i].kind == kind && timers->entries[i].window == window) {
            return true;
        }
    }
    return false;
}

void timers_run_due(wm_desktop *desktop)
{
    timer_entry due;
    desktop->timers.scheduled = false;
    while (find_due(&desktop->timers, timers_now(desktop), &due)) {
        timers_cancel(desktop, due.kind, due.window);
        dispatch(desktop, &due);
    }
}

void timers_reschedule(wm_desktop *desktop)
{
    timer_set *timers = &desktop->timers;
    uint32_t now = timers_now(desktop);
    uint32_t deadline;
    if (!earliest_deadline(timers, &deadline)) {
        return;
    }
    if (timers->scheduled && timers->scheduled_deadline_ms == deadline) {
        return;
    }
    timers->scheduled = true;
    timers->scheduled_deadline_ms = deadline;
    uint32_t delay = is_due(deadline, now) ? 0 : deadline - now;
    desktop->ports.commands.schedule_timer(desktop->ports.commands.self, delay);
}

uint32_t timers_now(const wm_desktop *desktop)
{
    return desktop->ports.environment.now_ms(desktop->ports.environment.self);
}

static bool is_due(uint32_t deadline_ms, uint32_t now_ms)
{
    return (int32_t) (deadline_ms - now_ms) <= 0;
}

static bool find_due(const timer_set *timers, uint32_t now_ms, timer_entry *due)
{
    for (size_t i = 0; i < timers->count; i++) {
        if (is_due(timers->entries[i].deadline_ms, now_ms)) {
            *due = timers->entries[i];
            return true;
        }
    }
    return false;
}

static void remove_at(timer_set *timers, size_t index)
{
    timers->entries[index] = timers->entries[timers->count - 1];
    timers->count--;
}

static bool grow(timer_set *timers)
{
    if (timers->count < timers->capacity) {
        return true;
    }
    size_t capacity = timers->capacity ? timers->capacity * TIMERS_GROWTH_FACTOR
                                       : TIMERS_INITIAL_CAPACITY;
    timer_entry *entries = realloc(timers->entries, capacity * sizeof(entries[0]));
    if (entries == NULL) {
        return false;
    }
    timers->entries = entries;
    timers->capacity = capacity;
    return true;
}

static void dispatch(wm_desktop *desktop, const timer_entry *entry)
{
    switch (entry->kind) {
    case TIMER_FOCUS:
        focus_on_delay_elapsed(desktop, entry->window);
        break;
    case TIMER_RAISE:
        focus_on_raise_delay_elapsed(desktop);
        break;
    case TIMER_BLINK:
        urgency_on_blink(desktop, entry->window);
        break;
    case TIMER_CLICK:
        click_on_timeout(desktop);
        break;
    case TIMER_LAUNCH:
        launches_on_timer(desktop);
        break;
    }
}

static bool earliest_deadline(const timer_set *timers, uint32_t *deadline_ms)
{
    if (timers->count == 0) {
        return false;
    }
    uint32_t best = timers->entries[0].deadline_ms;
    for (size_t i = 1; i < timers->count; i++) {
        if ((int32_t) (timers->entries[i].deadline_ms - best) < 0) {
            best = timers->entries[i].deadline_ms;
        }
    }
    *deadline_ms = best;
    return true;
}
