#include "click.h"
#include "cycle.h"
#include "desktop_state.h"
#include "focus.h"
#include "frame_buttons.h"
#include "highlight.h"
#include "initial_state.h"
#include "launches.h"
#include "menu.h"
#include "moveresize.h"
#include "placement.h"
#include "present.h"
#include "rules.h"
#include "stack.h"
#include "transients.h"

typedef struct map_request {
    wm_window_desc desc;
    bool launched;
} map_request;

static map_request launch_request(wm_desktop *desktop, const wm_window_desc *desc);
static void show_new(wm_desktop *desktop, wm_window *window);
static wm_window *attach(wm_desktop *desktop, const wm_window_desc *desc);
static int initial_workspace(const wm_desktop *desktop, wm_window *window,
                             const wm_window_desc *desc);
static int requested_workspace(const wm_desktop *desktop, const wm_window_desc *desc);
static void forget(wm_desktop *desktop, wm_window *window);
static void apply_user_time(const wm_desktop *desktop, wm_window *window,
                            const map_request *request);
static bool is_managed(const wm_desktop *desktop, const wm_window *window);

wm_window *wm_window_map(wm_desktop *desktop, const wm_window_desc *desc)
{
    map_request request = launch_request(desktop, desc);
    wm_window *window = attach(desktop, &request.desc);
    if (window == NULL) {
        return NULL;
    }
    window->mapped_ms = timers_now(desktop);
    apply_user_time(desktop, window, &request);
    show_new(desktop, window);
    return window;
}

void wm_window_unmap(wm_desktop *desktop, wm_window *window)
{
    window_list group = transient_list(desktop, window);
    if (desktop->focus.focused == window) {
        focus_pass_after_hide(desktop, window, &group);
    }
    window_list_free(&group);
    forget(desktop, window);
    stack_remove(desktop, window);
    transient_detach_all(desktop, window);
    stack_update_layers(desktop);
    window_destroy(window);
    present_flush(desktop);
}

wm_window_view wm_window_current_view(const wm_window *window)
{
    return window->view;
}

static wm_window *attach(wm_desktop *desktop, const wm_window_desc *desc)
{
    wm_window *window = window_create(desc, desktop->next_serial++);
    if (window == NULL) {
        return NULL;
    }
    if (desc->parent != NULL && is_managed(desktop, desc->parent)) {
        window->parent = desc->parent;
    }
    window_apply_type(window);
    window_update_flags(window);
    window->workspace = initial_workspace(desktop, window, desc);
    stack_add(desktop, window);
    window_list_append(&desktop->mru, window);
    return window;
}

static int initial_workspace(const wm_desktop *desktop, wm_window *window,
                             const wm_window_desc *desc)
{
    const wm_window *parent = window->parent;
    if (window_is_sticky(window) && !window_has_flag(window, WINDOW_HAS_STICK)
        && window->type != WM_TYPE_DESKTOP && window->type != WM_TYPE_DOCK) {
        window->states &= ~(uint32_t) WM_STATE_STICKY;
    }
    if (parent != NULL && window_is_sticky(parent)) {
        window->states |= WM_STATE_STICKY;
    }
    if (parent != NULL && !window_is_sticky(window)) {
        return parent->workspace;
    }
    return requested_workspace(desktop, desc);
}

static int requested_workspace(const wm_desktop *desktop, const wm_window_desc *desc)
{
    bool known = desc->workspace >= 0 && desc->workspace < desktop->workspaces.count;
    return desc->has_workspace && known ? desc->workspace : desktop->workspaces.current;
}

static map_request launch_request(wm_desktop *desktop, const wm_window_desc *desc)
{
    map_request request = {*desc, false};
    int workspace;
    request.launched = desc->parent == NULL && launches_take(desktop, desc->launch_id, &workspace);
    if (request.launched && !desc->has_workspace) {
        request.desc.has_workspace = true;
        request.desc.workspace = workspace;
    }
    return request;
}

static void show_new(wm_desktop *desktop, wm_window *window)
{
    placement_init_position(desktop, window);
    rules_apply_to_new(desktop, window);
    window->saved = window->client;
    window->pre_fullscreen = window->client;
    initial_state_apply(desktop, window);
    present_request_configure(window);
    initial_state_show(desktop, window);
    present_flush(desktop);
}

static void apply_user_time(const wm_desktop *desktop, wm_window *window,
                            const map_request *request)
{
    const wm_window_desc *desc = &request->desc;
    if (!desc->has_user_time) {
        return;
    }
    if (desc->user_time == 0) {
        window->skip_initial_focus = true;
        return;
    }
    bool older = !request->launched && desktop->has_user_input &&
                 (int32_t) (desc->user_time - desktop->last_user_input_ms) < 0;
    if (older) {
        window->activation_allowed = false;
    }
}

static void forget(wm_desktop *desktop, wm_window *window)
{
    timers_cancel_window(desktop, window);
    moveresize_window_gone(desktop, window);
    click_window_gone(desktop, window);
    frame_buttons_window_gone(desktop, window);
    cycle_window_gone(desktop, window);
    menu_window_gone(desktop, window);
    highlight_end(desktop, window);
    focus_forget(desktop, window);
}

static bool is_managed(const wm_desktop *desktop, const wm_window *window)
{
    return window_list_contains(&desktop->stack, window);
}
