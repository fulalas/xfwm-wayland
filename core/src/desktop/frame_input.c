#include "frame_input.h"

#include <stdlib.h>

#include "click.h"
#include "desktop_state.h"
#include "focus.h"
#include "frame_buttons.h"
#include "stack.h"
#include "workspaces.h"

enum {
    MIN_CORNER_PIXELS = 50,
    CORNER_FRACTION = 3,
};

static wm_dispatch easy_click_press(wm_desktop *desktop, wm_window *window,
                                    const wm_button_event *event);
static void easy_edge_press(wm_desktop *desktop, wm_window *window, const wm_button_event *event);
static void button1_action(wm_desktop *desktop, wm_window *window, const wm_button_event *event);
static void edge_button(wm_desktop *desktop, const click_request *request);
static wm_part edge_part(const wm_window *window, wm_point point);
static wm_part corner_part(const wm_rect *client, wm_point local);
static wm_dispatch frame_press(wm_desktop *desktop, wm_window *window,
                               const wm_button_event *event);
static void title_press(wm_desktop *desktop, wm_window *window, const wm_button_event *event);
static void begin_click(wm_desktop *desktop, click_purpose purpose,
                        const wm_button_event *event);
static wm_dispatch client_press(wm_desktop *desktop, wm_window *window,
                                const wm_button_event *event);
static bool is_button_part(wm_part part);

wm_dispatch frame_input_press(wm_desktop *desktop, const wm_button_event *event)
{
    wm_window *window = event->pointer.target.window;
    if (frame_input_is_easy_click(desktop, event->pointer.modifiers)) {
        wm_dispatch result = easy_click_press(desktop, window, event);
        if (result == WM_CONSUMED) {
            return result;
        }
    }
    if (event->pointer.target.kind == WM_TARGET_FRAME) {
        return frame_press(desktop, window, event);
    }
    return client_press(desktop, window, event);
}

wm_part frame_input_part_at(const wm_window *window, wm_point point)
{
    const wm_rect *frame = &window->view.frame;
    wm_point local = {point.x - frame->x, point.y - frame->y};
    return wm_decor_part_at(&window->layout, local);
}

wm_point frame_input_menu_anchor(const wm_window *window)
{
    wm_part part = (wm_part) (WM_PART_BUTTON_FIRST + WM_BUTTON_MENU);
    const wm_decor_layout *layout = &window->layout;
    for (size_t i = 0; i < layout->part_count; i++) {
        if (layout->parts[i].part == part) {
            return (wm_point) {window->view.frame.x + layout->parts[i].rect.x,
                               window->client.y};
        }
    }
    return (wm_point) {window->client.x, window->client.y};
}

bool frame_input_is_easy_click(const wm_desktop *desktop, uint32_t modifiers)
{
    return modifiers != 0 && modifiers == desktop->config.mouse.easy_click_modifiers;
}

static wm_dispatch easy_click_press(wm_desktop *desktop, wm_window *window,
                                    const wm_button_event *event)
{
    switch (event->button) {
    case WM_MOUSE_LEFT:
        button1_action(desktop, window, event);
        return WM_CONSUMED;
    case WM_MOUSE_MIDDLE:
        stack_lower(desktop, window);
        return WM_CONSUMED;
    case WM_MOUSE_RIGHT:
        easy_edge_press(desktop, window, event);
        return WM_CONSUMED;
    case WM_MOUSE_BACK:
        workspaces_switch(desktop, desktop->workspaces.current - 1, NULL);
        return WM_CONSUMED;
    case WM_MOUSE_FORWARD:
        workspaces_switch(desktop, desktop->workspaces.current + 1, NULL);
        return WM_CONSUMED;
    default:
        return WM_FORWARD;
    }
}

static void easy_edge_press(wm_desktop *desktop, wm_window *window, const wm_button_event *event)
{
    wm_part part = edge_part(window, event->pointer.position);
    click_request request = {CLICK_FOR_EDGE, window, part, event};
    edge_button(desktop, &request);
}

static void button1_action(wm_desktop *desktop, wm_window *window, const wm_button_event *event)
{
    if (!window_is_dont_focus(window)) {
        focus_set(desktop, window);
    }
    stack_raise(desktop, window);
    click_request request = {CLICK_FOR_TITLE, window, WM_PART_TITLE, event};
    click_begin(desktop, &request);
}

static void edge_button(wm_desktop *desktop, const click_request *request)
{
    const wm_button_event *event = request->press;
    wm_window *window = request->window;
    if (event->button == WM_MOUSE_MIDDLE) {
        click_request move = {CLICK_FOR_EDGE_MOVE, window, request->part, event};
        click_begin(desktop, &move);
        return;
    }
    if (event->button != WM_MOUSE_LEFT && event->button != WM_MOUSE_RIGHT) {
        return;
    }
    bool easy = frame_input_is_easy_click(desktop, event->pointer.modifiers);
    if (event->button == WM_MOUSE_LEFT || easy) {
        if (!window_is_dont_focus(window)) {
            focus_set(desktop, window);
        }
        stack_raise(desktop, window);
    }
    click_begin(desktop, request);
}

static wm_part edge_part(const wm_window *window, wm_point point)
{
    const wm_rect *client = &window->client;
    wm_point local = {point.x - client->x, point.y - client->y};
    int corner_x = wm_max(client->width / CORNER_FRACTION, MIN_CORNER_PIXELS);
    int corner_y = wm_max(client->height / CORNER_FRACTION, MIN_CORNER_PIXELS);
    int distance_x = client->width / 2 - abs(client->width / 2 - local.x);
    int distance_y = client->height / 2 - abs(client->height / 2 - local.y);
    if (distance_x < corner_x && distance_y < corner_y) {
        return corner_part(client, local);
    }
    if (distance_x / corner_x < distance_y / corner_y) {
        return local.x < client->width / 2 ? WM_PART_SIDE_LEFT : WM_PART_SIDE_RIGHT;
    }
    return local.y < client->height / 2 ? WM_PART_SIDE_TOP : WM_PART_SIDE_BOTTOM;
}

static wm_part corner_part(const wm_rect *client, wm_point local)
{
    bool top = local.y < client->height / 2;
    if (local.x < client->width / 2) {
        return top ? WM_PART_CORNER_TOP_LEFT : WM_PART_CORNER_BOTTOM_LEFT;
    }
    return top ? WM_PART_CORNER_TOP_RIGHT : WM_PART_CORNER_BOTTOM_RIGHT;
}

static wm_dispatch frame_press(wm_desktop *desktop, wm_window *window,
                               const wm_button_event *event)
{
    wm_part part = frame_input_part_at(window, event->pointer.position);
    if (is_button_part(part) && part != WM_PART_BUTTON_FIRST + WM_BUTTON_MENU) {
        if (event->button <= WM_MOUSE_RIGHT) {
            focus_and_raise_on_click(desktop, window);
            frame_buttons_press(desktop, (wm_button) (part - WM_PART_BUTTON_FIRST), event);
        }
    } else if (part == WM_PART_TITLE) {
        title_press(desktop, window, event);
    } else if (part == WM_PART_BUTTON_FIRST + WM_BUTTON_MENU) {
        if (event->button == WM_MOUSE_LEFT) {
            begin_click(desktop, CLICK_FOR_MENU_BUTTON, event);
        }
    } else if (part >= WM_PART_CORNER_BOTTOM_LEFT && part <= WM_PART_SIDE_BOTTOM) {
        click_request request = {CLICK_FOR_EDGE, window, part, event};
        edge_button(desktop, &request);
    }
    return WM_CONSUMED;
}

static void title_press(wm_desktop *desktop, wm_window *window, const wm_button_event *event)
{
    if (event->button == WM_MOUSE_LEFT) {
        button1_action(desktop, window, event);
    } else if (event->button == WM_MOUSE_MIDDLE) {
        stack_lower(desktop, window);
    } else if (event->button == WM_MOUSE_RIGHT) {
        begin_click(desktop, CLICK_FOR_TITLE_MENU, event);
    }
}

static void begin_click(wm_desktop *desktop, click_purpose purpose,
                        const wm_button_event *event)
{
    wm_window *window = event->pointer.target.window;
    click_request request = {purpose, window, WM_PART_TITLE, event};
    click_begin(desktop, &request);
}

static wm_dispatch client_press(wm_desktop *desktop, wm_window *window,
                                const wm_button_event *event)
{
    bool any_button = desktop->config.focus.raise_with_any_button
                      && window_is_regular_focusable(window);
    if (any_button || event->button == WM_MOUSE_LEFT) {
        focus_and_raise_on_click(desktop, window);
    }
    return WM_FORWARD;
}

static bool is_button_part(wm_part part)
{
    return part >= WM_PART_BUTTON_FIRST && part <= WM_PART_BUTTON_LAST;
}
