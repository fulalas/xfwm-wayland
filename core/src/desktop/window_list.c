#include "window_list.h"

#include <stdlib.h>
#include <string.h>

enum {
    LIST_INITIAL_CAPACITY = 8,
    LIST_GROWTH_FACTOR = 2,
};

static bool ensure_capacity(window_list *list);

void window_list_free(window_list *list)
{
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

bool window_list_append(window_list *list, wm_window *window)
{
    return window_list_insert(list, list->count, window);
}

bool window_list_insert(window_list *list, size_t index, wm_window *window)
{
    if (!ensure_capacity(list)) {
        return false;
    }
    if (index > list->count) {
        index = list->count;
    }
    memmove(&list->items[index + 1], &list->items[index],
            (list->count - index) * sizeof(list->items[0]));
    list->items[index] = window;
    list->count++;
    return true;
}

void window_list_remove(window_list *list, const wm_window *window)
{
    size_t index = window_list_index_of(list, window);
    if (index == list->count) {
        return;
    }
    memmove(&list->items[index], &list->items[index + 1],
            (list->count - index - 1) * sizeof(list->items[0]));
    list->count--;
}

bool window_list_contains(const window_list *list, const wm_window *window)
{
    return window_list_index_of(list, window) < list->count;
}

size_t window_list_index_of(const window_list *list, const wm_window *window)
{
    for (size_t i = 0; i < list->count; i++) {
        if (list->items[i] == window) {
            return i;
        }
    }
    return list->count;
}

void window_list_move_to_front(window_list *list, wm_window *window)
{
    if (window_list_contains(list, window)) {
        window_list_remove(list, window);
        window_list_insert(list, 0, window);
    }
}

void window_list_move_to_back(window_list *list, wm_window *window)
{
    if (window_list_contains(list, window)) {
        window_list_remove(list, window);
        window_list_append(list, window);
    }
}

static bool ensure_capacity(window_list *list)
{
    if (list->count < list->capacity) {
        return true;
    }
    size_t capacity = list->capacity ? list->capacity * LIST_GROWTH_FACTOR : LIST_INITIAL_CAPACITY;
    wm_window **items = realloc(list->items, capacity * sizeof(items[0]));
    if (items == NULL) {
        return false;
    }
    list->items = items;
    list->capacity = capacity;
    return true;
}
