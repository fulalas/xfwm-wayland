#include "click.h"

#include <stdlib.h>

#include "desktop_state.h"
#include "moveresize.h"
#include "timers.h"

static bool allows_double(click_purpose purpose);
static bool timed_out(const wm_desktop *desktop, uint32_t time_ms);
static bool is_terminal(const click_state *click);
static void finish(wm_desktop *desktop);

void click_begin(wm_desktop *desktop, const click_request *request)
{
    click_state *click = &desktop->click;
    click->pending = true;
    click->purpose = request->purpose;
    click->window = request->window;
    click->part = request->part;
    click->button = request->press->button;
    click->clicks = 1;
    click->origin = request->press->pointer.position;
    click->start_ms = request->press->pointer.time_ms;
    timer_request timer = {TIMER_CLICK, NULL, desktop->config.mouse.double_click_time_ms};
    timers_start(desktop, &timer);
}

bool click_is_pending(const wm_desktop *desktop)
{
    return desktop->click.pending;
}

wm_dispatch click_motion(wm_desktop *desktop, const wm_pointer_event *event)
{
    click_state *click = &desktop->click;
    int distance = desktop->config.mouse.double_click_distance;
    bool moved = abs(event->position.x - click->origin.x) > distance
                 || abs(event->position.y - click->origin.y) > distance;
    if (moved || timed_out(desktop, event->time_ms)) {
        finish(desktop);
        if (moveresize_is_active(desktop)) {
            moveresize_motion(desktop, event);
        }
    }
    return WM_CONSUMED;
}

wm_dispatch click_button(wm_desktop *desktop, const wm_button_event *event)
{
    click_state *click = &desktop->click;
    bool expired = timed_out(desktop, event->pointer.time_ms);
    if (event->button == click->button) {
        click->clicks++;
    }
    if (expired || is_terminal(click)) {
        finish(desktop);
    }
    return WM_CONSUMED;
}

void click_on_timeout(wm_desktop *desktop)
{
    if (desktop->click.pending) {
        finish(desktop);
    }
}

void click_window_gone(wm_desktop *desktop, const wm_window *window)
{
    if (desktop->click.pending && desktop->click.window == window) {
        desktop->click.pending = false;
        timers_cancel_kind(desktop, TIMER_CLICK);
    }
}

static bool allows_double(click_purpose purpose)
{
    return purpose == CLICK_FOR_TITLE || purpose == CLICK_FOR_EDGE
           || purpose == CLICK_FOR_MENU_BUTTON;
}

static bool timed_out(const wm_desktop *desktop, uint32_t time_ms)
{
    int elapsed = (int) (time_ms - desktop->click.start_ms);
    return elapsed > desktop->config.mouse.double_click_time_ms;
}

static bool is_terminal(const click_state *click)
{
    if (click->clicks >= CLICK_DOUBLE) {
        return true;
    }
    return !allows_double(click->purpose) && click->clicks == CLICK_SINGLE;
}

static void finish(wm_desktop *desktop)
{
    click_state click = desktop->click;
    desktop->click.pending = false;
    timers_cancel_kind(desktop, TIMER_CLICK);
    click_type type = click.clicks > CLICK_DOUBLE ? CLICK_DOUBLE : (click_type) click.clicks;
    click_finished(desktop, &click, type);
}
