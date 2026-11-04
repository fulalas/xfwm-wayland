#include "stack.h"

#include "desktop_state.h"
#include "focus.h"
#include "frame.h"
#include "transients.h"

static void raise_internal(wm_desktop *desktop, wm_window *window);
static void restack_relayered(wm_desktop *desktop, wm_window *window);
static size_t top_of_layer(const window_list *stack, wm_layer layer);
static size_t bottom_of_layer(const window_list *stack, wm_layer layer);
static wm_layer base_layer(const wm_desktop *desktop, const wm_window *window);
static wm_layer computed_layer(const wm_desktop *desktop, const wm_window *window);
static bool fullscreen_has_focus(const wm_desktop *desktop, const wm_window *window);
static void sort_by_layer(window_list *stack);
static void place_above(wm_desktop *desktop, wm_window *window, const wm_window *sibling);
static void stack_mark_changed(wm_desktop *desktop);
static window_list snapshot(const window_list *list);
static bool same_order(const window_list *a, const window_list *b);

void stack_add(wm_desktop *desktop, wm_window *window)
{
    window->layer = computed_layer(desktop, window);
    window_list_insert(&desktop->stack, top_of_layer(&desktop->stack, window->layer), window);
    stack_mark_changed(desktop);
}

void stack_remove(wm_desktop *desktop, wm_window *window)
{
    window_list_remove(&desktop->stack, window);
    if (desktop->focus.last_raise == window) {
        desktop->focus.last_raise = NULL;
    }
    stack_mark_changed(desktop);
}

void stack_raise(wm_desktop *desktop, wm_window *window)
{
    window_list before = snapshot(&desktop->stack);
    wm_window *ancestor = transient_root(window);
    raise_internal(desktop, ancestor);
    if (ancestor != window) {
        raise_internal(desktop, window);
    }
    desktop->focus.last_raise = window;
    if (!same_order(&before, &desktop->stack)) {
        stack_mark_changed(desktop);
    }
    window_list_free(&before);
}

void stack_lower(wm_desktop *desktop, wm_window *window)
{
    window_list *stack = &desktop->stack;
    window_list_remove(stack, window);
    if (window->parent != NULL && window->parent->layer == window->layer
        && window_list_contains(stack, window->parent)) {
        place_above(desktop, window, window->parent);
    } else {
        window_list_insert(stack, bottom_of_layer(stack, window->layer), window);
    }
    if (desktop->focus.last_raise == window) {
        desktop->focus.last_raise = NULL;
    }
    stack_mark_changed(desktop);
    focus_pass(desktop, window);
}

void stack_update_layers(wm_desktop *desktop)
{
    window_list changed = {0};
    window_list *stack = &desktop->stack;
    for (size_t i = 0; i < stack->count; i++) {
        wm_layer layer = computed_layer(desktop, stack->items[i]);
        if (layer != stack->items[i]->layer) {
            stack->items[i]->layer = layer;
            window_mark_dirty(stack->items[i]);
            window_list_append(&changed, stack->items[i]);
        }
    }
    if (changed.count > 0) {
        sort_by_layer(stack);
        stack_mark_changed(desktop);
    }
    for (size_t i = 0; i < changed.count; i++) {
        restack_relayered(desktop, changed.items[i]);
    }
    window_list_free(&changed);
}

bool stack_layers_outdated(const wm_desktop *desktop)
{
    const window_list *stack = &desktop->stack;
    for (size_t i = 0; i < stack->count; i++) {
        if (computed_layer(desktop, stack->items[i]) != stack->items[i]->layer) {
            return true;
        }
    }
    return false;
}

void stack_place_below(wm_desktop *desktop, wm_window *window, const wm_window *sibling)
{
    window_list *stack = &desktop->stack;
    window_list_remove(stack, window);
    if (sibling->layer == window->layer && window_list_contains(stack, sibling)) {
        window_list_insert(stack, window_list_index_of(stack, sibling), window);
    } else {
        window_list_insert(stack, bottom_of_layer(stack, window->layer), window);
    }
    stack_mark_changed(desktop);
}

bool stack_is_top_most(const wm_desktop *desktop, const wm_window *window)
{
    const window_list *stack = &desktop->stack;
    for (size_t i = window_list_index_of(stack, window) + 1; i < stack->count; i++) {
        if (stack->items[i]->visible && stack->items[i]->layer == window->layer) {
            return false;
        }
    }
    return true;
}

static void raise_internal(wm_desktop *desktop, wm_window *window)
{
    window_list moved = {0};
    window_list *stack = &desktop->stack;
    window_list group = transient_list(desktop, window);
    for (size_t i = 0; i < group.count; i++) {
        if (group.items[i] == window || group.items[i]->layer <= window->layer) {
            window_list_append(&moved, group.items[i]);
        }
    }
    for (size_t i = 0; i < moved.count; i++) {
        window_list_remove(stack, moved.items[i]);
    }
    size_t index = top_of_layer(stack, window->layer);
    for (size_t i = 0; i < moved.count; i++) {
        window_list_insert(stack, index + i, moved.items[i]);
    }
    window_list_free(&group);
    window_list_free(&moved);
}

static void restack_relayered(wm_desktop *desktop, wm_window *window)
{
    wm_window *focused = desktop->focus.focused;
    if (focused == NULL || focused == window || focused->layer != window->layer) {
        raise_internal(desktop, window);
        return;
    }
    wm_rect frame = frame_rect(desktop, window);
    wm_rect focused_frame = frame_rect(desktop, focused);
    if (wm_rect_intersects(&frame, &focused_frame)) {
        stack_place_below(desktop, window, focused);
    } else {
        raise_internal(desktop, window);
    }
}

static size_t top_of_layer(const window_list *stack, wm_layer layer)
{
    for (size_t i = 0; i < stack->count; i++) {
        if (stack->items[i]->layer > layer) {
            return i;
        }
    }
    return stack->count;
}

static size_t bottom_of_layer(const window_list *stack, wm_layer layer)
{
    for (size_t i = 0; i < stack->count; i++) {
        if (stack->items[i]->layer >= layer) {
            return i;
        }
    }
    return stack->count;
}

static wm_layer base_layer(const wm_desktop *desktop, const wm_window *window)
{
    if (window->type == WM_TYPE_NOTIFICATION) {
        return WM_LAYER_FULLSCREEN;
    }
    if (window->type == WM_TYPE_DESKTOP) {
        return WM_LAYER_DESKTOP;
    }
    if (window->type == WM_TYPE_DOCK) {
        return WM_LAYER_DOCK;
    }
    if (window_has_state(window, WM_STATE_ABOVE)) {
        return WM_LAYER_ABOVE;
    }
    if (window_has_state(window, WM_STATE_BELOW)) {
        return WM_LAYER_BELOW;
    }
    if (fullscreen_has_focus(desktop, window)) {
        return WM_LAYER_FULLSCREEN;
    }
    return WM_LAYER_NORMAL;
}

static wm_layer computed_layer(const wm_desktop *desktop, const wm_window *window)
{
    wm_layer layer = base_layer(desktop, window);
    if (window->parent != NULL) {
        wm_layer parent_layer = computed_layer(desktop, window->parent);
        if (parent_layer > layer) {
            layer = parent_layer;
        }
    }
    return layer;
}

static bool fullscreen_has_focus(const wm_desktop *desktop, const wm_window *window)
{
    const wm_window *focused = desktop->focus.focused;
    if (!window_is_fullscreen(window) || focused == NULL) {
        return false;
    }
    return focused == window || transient_is_descendant_of(focused, window);
}

static void sort_by_layer(window_list *stack)
{
    for (size_t i = 1; i < stack->count; i++) {
        wm_window *window = stack->items[i];
        size_t j = i;
        while (j > 0 && stack->items[j - 1]->layer > window->layer) {
            stack->items[j] = stack->items[j - 1];
            j--;
        }
        stack->items[j] = window;
    }
}

static void place_above(wm_desktop *desktop, wm_window *window, const wm_window *sibling)
{
    size_t index = window_list_index_of(&desktop->stack, sibling) + 1;
    window_list_insert(&desktop->stack, index, window);
}

static void stack_mark_changed(wm_desktop *desktop)
{
    desktop->present.stacking_dirty = true;
}

static window_list snapshot(const window_list *list)
{
    window_list copy = {0};
    for (size_t i = 0; i < list->count; i++) {
        window_list_append(&copy, list->items[i]);
    }
    return copy;
}

static bool same_order(const window_list *a, const window_list *b)
{
    if (a->count != b->count) {
        return false;
    }
    for (size_t i = 0; i < a->count; i++) {
        if (a->items[i] != b->items[i]) {
            return false;
        }
    }
    return true;
}
