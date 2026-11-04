#include "transients.h"

#include <string.h>

#include "desktop_state.h"

static bool is_listed_descendant(const window_list *listed, const wm_window *window);

bool transient_is_for(const wm_window *transient, const wm_window *parent)
{
    return transient != parent && transient->parent == parent;
}

bool transient_is_descendant_of(const wm_window *window, const wm_window *ancestor)
{
    for (const wm_window *w = window->parent; w != NULL; w = w->parent) {
        if (w == ancestor) {
            return true;
        }
    }
    return false;
}

bool transient_is_modal(const wm_window *window)
{
    return window_has_state(window, WM_STATE_MODAL) && window_is_regular_focusable(window)
           && window->parent != NULL;
}

bool transient_is_modal_for(const wm_window *modal, const wm_window *parent)
{
    return transient_is_modal(modal) && modal->serial >= parent->serial
           && transient_is_for(modal, parent);
}

bool transient_same_application(const wm_window *a, const wm_window *b)
{
    if (a == b) {
        return false;
    }
    if (transient_is_for(a, b) || transient_is_for(b, a)) {
        return true;
    }
    return a->app_id != NULL && b->app_id != NULL && a->app_id[0] != '\0'
           && strcmp(a->app_id, b->app_id) == 0;
}

bool transient_parent_allowed(const wm_window *window, const wm_window *parent)
{
    for (const wm_window *w = parent; w != NULL; w = w->parent) {
        if (w == window) {
            return false;
        }
    }
    return true;
}

wm_window *transient_modal_for(const wm_desktop *desktop, const wm_window *window)
{
    const window_list *stack = &desktop->stack;
    for (size_t i = stack->count; i > 0; i--) {
        wm_window *candidate = stack->items[i - 1];
        if (candidate != window && transient_is_modal_for(candidate, window)) {
            return candidate;
        }
    }
    return NULL;
}

wm_window *transient_root(wm_window *window)
{
    wm_window *root = window;
    for (wm_window *w = window->parent; w != NULL; w = w->parent) {
        if (window->layer > w->layer) {
            break;
        }
        root = w;
    }
    return root;
}

window_list transient_list(const wm_desktop *desktop, wm_window *window)
{
    window_list list = {0};
    window_list_append(&list, window);
    const window_list *stack = &desktop->stack;
    for (size_t i = 0; i < stack->count; i++) {
        wm_window *candidate = stack->items[i];
        if (candidate != window && is_listed_descendant(&list, candidate)) {
            window_list_append(&list, candidate);
        }
    }
    return list;
}

void transient_detach_all(wm_desktop *desktop, const wm_window *window)
{
    const window_list *stack = &desktop->stack;
    for (size_t i = 0; i < stack->count; i++) {
        if (stack->items[i]->parent == window) {
            stack->items[i]->parent = NULL;
            window_update_flags(stack->items[i]);
        }
    }
}

static bool is_listed_descendant(const window_list *listed, const wm_window *window)
{
    for (const wm_window *w = window->parent; w != NULL; w = w->parent) {
        if (window_list_contains(listed, w)) {
            return true;
        }
    }
    return false;
}
