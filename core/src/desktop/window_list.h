#ifndef DESKTOP_WINDOW_LIST_H
#define DESKTOP_WINDOW_LIST_H

#include <stdbool.h>
#include <stddef.h>

#include "wm/desktop.h"

typedef struct window_list {
    wm_window **items;
    size_t count;
    size_t capacity;
} window_list;

void window_list_free(window_list *list);
bool window_list_append(window_list *list, wm_window *window);
bool window_list_insert(window_list *list, size_t index, wm_window *window);
void window_list_remove(window_list *list, const wm_window *window);
bool window_list_contains(const window_list *list, const wm_window *window);
size_t window_list_index_of(const window_list *list, const wm_window *window);
void window_list_move_to_front(window_list *list, wm_window *window);
void window_list_move_to_back(window_list *list, wm_window *window);

#endif
