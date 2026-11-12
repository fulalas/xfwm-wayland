#ifndef DESKTOP_CLICK_H
#define DESKTOP_CLICK_H

#include <stdbool.h>
#include <stdint.h>

#include "wm/desktop.h"

typedef enum click_type {
    CLICK_UNDEFINED = 0,
    CLICK_DRAG = 1,
    CLICK_SINGLE = 2,
    CLICK_AND_DRAG = 3,
    CLICK_DOUBLE = 4,
} click_type;

typedef enum click_purpose {
    CLICK_FOR_TITLE,
    CLICK_FOR_TITLE_MENU,
    CLICK_FOR_EDGE,
    CLICK_FOR_EDGE_MOVE,
    CLICK_FOR_MENU_BUTTON,
} click_purpose;

typedef struct click_state {
    bool pending;
    click_purpose purpose;
    wm_window *window;
    wm_part part;
    uint32_t button;
    unsigned clicks;
    wm_point origin;
    uint32_t start_ms;
} click_state;

typedef struct click_request {
    click_purpose purpose;
    wm_window *window;
    wm_part part;
    const wm_button_event *press;
} click_request;

void click_begin(wm_desktop *desktop, const click_request *request);
bool click_is_pending(const wm_desktop *desktop);
wm_dispatch click_motion(wm_desktop *desktop, const wm_pointer_event *event);
wm_dispatch click_button(wm_desktop *desktop, const wm_button_event *event);
void click_on_timeout(wm_desktop *desktop);
void click_window_gone(wm_desktop *desktop, const wm_window *window);
void click_finished(wm_desktop *desktop, const click_state *click, click_type type);

#endif
