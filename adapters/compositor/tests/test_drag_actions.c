#include <linux/input-event-codes.h>
#include <wayland-server-protocol.h>

#include "check.h"
#include "drag_actions.h"

static const int32_t COPY = WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY;
static const int32_t MOVE = WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE;
static const int32_t ASK = WL_DATA_DEVICE_MANAGER_DND_ACTION_ASK;

static int32_t offered(int32_t actions, uint32_t button, bool alt_held)
{
    drag_start start = {actions, button, alt_held};
    return drag_actions_offered(&start);
}

static void test_left_drag_never_asks(void)
{
    CHECK_INT_EQ(COPY | MOVE, offered(COPY | MOVE | ASK, BTN_LEFT, false));
}

static void test_touch_drag_never_asks(void)
{
    CHECK_INT_EQ(COPY | MOVE, offered(COPY | MOVE | ASK, 0, false));
}

static void test_right_and_middle_drags_keep_ask(void)
{
    CHECK_INT_EQ(COPY | MOVE | ASK, offered(COPY | MOVE | ASK, BTN_RIGHT, false));
    CHECK_INT_EQ(COPY | MOVE | ASK, offered(COPY | MOVE | ASK, BTN_MIDDLE, false));
}

static void test_alt_keeps_ask(void)
{
    CHECK_INT_EQ(COPY | MOVE | ASK, offered(COPY | MOVE | ASK, BTN_LEFT, true));
}

static void test_ask_only_source_keeps_ask(void)
{
    CHECK_INT_EQ(ASK, offered(ASK, BTN_LEFT, false));
}

static void test_unset_actions_stay_unset(void)
{
    CHECK_INT_EQ(-1, offered(-1, BTN_LEFT, false));
}

int main(void)
{
    const check_case cases[] = {
        {"left_drag_never_asks", test_left_drag_never_asks},
        {"touch_drag_never_asks", test_touch_drag_never_asks},
        {"right_and_middle_drags_keep_ask", test_right_and_middle_drags_keep_ask},
        {"alt_keeps_ask", test_alt_keeps_ask},
        {"ask_only_source_keeps_ask", test_ask_only_source_keeps_ask},
        {"unset_actions_stay_unset", test_unset_actions_stay_unset},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
