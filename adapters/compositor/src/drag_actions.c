#include "drag_actions.h"

#include <linux/input-event-codes.h>
#include <strings.h>
#include <wayland-server-protocol.h>

#include "wm/keys.h"

enum {
    ACTIONS_UNSET = -1,
};

static bool wants_ask(const drag_start *start);

int32_t drag_actions_offered(const drag_start *start)
{
    if (start->source_actions == ACTIONS_UNSET || wants_ask(start)) {
        return start->source_actions;
    }
    int32_t without_ask = start->source_actions & ~(int32_t) WL_DATA_DEVICE_MANAGER_DND_ACTION_ASK;
    return without_ask != 0 ? without_ask : start->source_actions;
}

int32_t drag_actions_for_modifiers(int32_t offered, uint32_t modifiers)
{
    if (offered == ACTIONS_UNSET
        || (offered & (int32_t) WL_DATA_DEVICE_MANAGER_DND_ACTION_ASK) != 0) {
        return offered;
    }
    uint32_t held = modifiers & (WM_MOD_CTRL | WM_MOD_SHIFT);
    int32_t wanted = WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE;
    if (held == WM_MOD_CTRL) {
        wanted = WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY;
    } else if (held == WM_MOD_SHIFT) {
        wanted = WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE;
    }
    return (offered & wanted) != 0 ? wanted : offered;
}

uint32_t drag_actions_choose(const drag_choice *choice)
{
    uint32_t offer_actions = choice->offer_sets_actions ? choice->offer_actions
                                                        : WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY;
    uint32_t preferred = choice->offer_sets_actions ? choice->preferred_action : 0;
    uint32_t source_actions = choice->source_actions != ACTIONS_UNSET
                                  ? (uint32_t) choice->source_actions
                                  : WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY;
    uint32_t available = offer_actions & source_actions;
    if (available == 0) {
        return WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE;
    }
    if ((preferred & available) != 0) {
        return preferred;
    }
    return 1u << (ffs((int) available) - 1);
}

static bool wants_ask(const drag_start *start)
{
    return start->alt_held || start->button == BTN_RIGHT || start->button == BTN_MIDDLE;
}
