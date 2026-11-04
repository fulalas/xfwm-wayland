#include "drag_actions.h"

#include <linux/input-event-codes.h>
#include <wayland-server-protocol.h>

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

static bool wants_ask(const drag_start *start)
{
    return start->alt_held || start->button == BTN_RIGHT || start->button == BTN_MIDDLE;
}
