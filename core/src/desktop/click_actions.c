#include "click.h"

#include "desktop_state.h"
#include "fill.h"
#include "focus.h"
#include "frame_input.h"
#include "menu.h"
#include "moveresize.h"
#include "stack.h"
#include "window_ops.h"

static void title_action(wm_desktop *desktop, const click_state *click, click_type type);
static void title_menu_action(wm_desktop *desktop, const click_state *click, click_type type);
static void edge_action(wm_desktop *desktop, const click_state *click, click_type type);
static void edge_move_action(wm_desktop *desktop, const click_state *click, click_type type);
static void menu_button_action(wm_desktop *desktop, const click_state *click, click_type type);
static void start_move(wm_desktop *desktop, const click_state *click);
static unsigned fill_mode_for(wm_part part);

void click_finished(wm_desktop *desktop, const click_state *click, click_type type)
{
    switch (click->purpose) {
    case CLICK_FOR_TITLE:
        title_action(desktop, click, type);
        break;
    case CLICK_FOR_TITLE_MENU:
        title_menu_action(desktop, click, type);
        break;
    case CLICK_FOR_EDGE:
        edge_action(desktop, click, type);
        break;
    case CLICK_FOR_EDGE_MOVE:
        edge_move_action(desktop, click, type);
        break;
    case CLICK_FOR_MENU_BUTTON:
        menu_button_action(desktop, click, type);
        break;
    }
}

static void title_action(wm_desktop *desktop, const click_state *click, click_type type)
{
    if (type == CLICK_DRAG || type == CLICK_AND_DRAG) {
        start_move(desktop, click);
    } else if (type == CLICK_DOUBLE) {
        window_ops_double_click(desktop, click->window);
    }
}

static void title_menu_action(wm_desktop *desktop, const click_state *click, click_type type)
{
    wm_window *window = click->window;
    if (type == CLICK_DRAG) {
        start_move(desktop, click);
        return;
    }
    if (type == CLICK_UNDEFINED) {
        return;
    }
    if (!window_is_dont_focus(window)) {
        focus_set(desktop, window);
    }
    if (desktop->config.focus.raise_on_click) {
        stack_raise(desktop, window);
    }
    menu_show(desktop, window, desktop->pointer);
}

static void edge_action(wm_desktop *desktop, const click_state *click, click_type type)
{
    if (type == CLICK_DOUBLE) {
        fill_window(desktop, click->window, fill_mode_for(click->part));
    } else if (type == CLICK_DRAG) {
        session_start start = {click->window, click->part, click->origin, click->button};
        moveresize_begin_pointer_resize(desktop, &start);
    }
}

static void edge_move_action(wm_desktop *desktop, const click_state *click, click_type type)
{
    if (type == CLICK_SINGLE) {
        stack_lower(desktop, click->window);
    } else if (type == CLICK_DRAG) {
        start_move(desktop, click);
    }
}

static void menu_button_action(wm_desktop *desktop, const click_state *click, click_type type)
{
    if (type == CLICK_DOUBLE) {
        window_ops_close(desktop, click->window);
        return;
    }
    if (type == CLICK_UNDEFINED) {
        return;
    }
    focus_and_raise_on_click(desktop, click->window);
    menu_show(desktop, click->window, frame_input_menu_anchor(click->window));
}

static void start_move(wm_desktop *desktop, const click_state *click)
{
    session_start start = {click->window, WM_PART_NONE, click->origin, click->button};
    moveresize_begin_pointer_move(desktop, &start);
}

static unsigned fill_mode_for(wm_part part)
{
    switch (part) {
    case WM_PART_SIDE_LEFT:
    case WM_PART_SIDE_RIGHT:
        return FILL_HORIZONTAL;
    case WM_PART_SIDE_TOP:
    case WM_PART_SIDE_BOTTOM:
        return FILL_VERTICAL;
    default:
        return FILL_BOTH;
    }
}
