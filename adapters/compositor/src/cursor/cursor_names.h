#ifndef COMPOSITOR_CURSOR_NAMES_H
#define COMPOSITOR_CURSOR_NAMES_H

#include <stdint.h>

#include "wm/views.h"

enum {
    CURSOR_NAME_CHOICES = 4,
};

typedef struct cursor_name_list {
    const char *names[CURSOR_NAME_CHOICES];
} cursor_name_list;

cursor_name_list cursor_names_for_shape(wm_cursor_shape shape);
cursor_name_list cursor_names_for_drag_action(uint32_t action);

#endif
