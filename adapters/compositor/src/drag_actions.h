#ifndef COMPOSITOR_DRAG_ACTIONS_H
#define COMPOSITOR_DRAG_ACTIONS_H

#include <stdbool.h>
#include <stdint.h>

typedef struct drag_start {
    int32_t source_actions;
    uint32_t button;
    bool alt_held;
} drag_start;

typedef struct drag_choice {
    bool offer_sets_actions;
    uint32_t offer_actions;
    uint32_t preferred_action;
    int32_t source_actions;
    uint32_t compositor_action;
} drag_choice;

int32_t drag_actions_offered(const drag_start *start);
uint32_t drag_action_for_modifiers(int32_t source_actions, uint32_t modifiers);
uint32_t drag_actions_choose(const drag_choice *choice);

#endif
