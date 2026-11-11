#ifndef DESKTOP_RULES_H
#define DESKTOP_RULES_H

#include <stddef.h>

#include "wm/desktop.h"

typedef struct rule_set {
    wm_window_rule *items;
    size_t count;
} rule_set;

void rules_apply_to_new(wm_desktop *desktop, wm_window *window);
void rules_free(rule_set *set);

#endif
