#ifndef DESKTOP_TIMERS_H
#define DESKTOP_TIMERS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "wm/desktop.h"

typedef enum timer_kind {
    TIMER_FOCUS,
    TIMER_RAISE,
    TIMER_BLINK,
    TIMER_CLICK,
    TIMER_LAUNCH,
} timer_kind;

typedef struct timer_entry {
    timer_kind kind;
    wm_window *window;
    uint32_t deadline_ms;
} timer_entry;

typedef struct timer_set {
    timer_entry *entries;
    size_t count;
    size_t capacity;
    bool scheduled;
    uint32_t scheduled_deadline_ms;
} timer_set;

void timers_free(timer_set *timers);
void timers_start(wm_desktop *desktop, timer_kind kind, wm_window *window, int delay_ms);
void timers_cancel(wm_desktop *desktop, timer_kind kind, wm_window *window);
void timers_cancel_kind(wm_desktop *desktop, timer_kind kind);
void timers_cancel_window(wm_desktop *desktop, const wm_window *window);
bool timers_is_running(const wm_desktop *desktop, timer_kind kind, const wm_window *window);
void timers_run_due(wm_desktop *desktop);
void timers_reschedule(wm_desktop *desktop);
uint32_t timers_now(const wm_desktop *desktop);

#endif
