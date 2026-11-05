#include <linux/input-event-codes.h>
#include <wayland-server-protocol.h>

#include "check.h"
#include "drag_actions.h"
#include "wm/keys.h"

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

static int32_t narrowed(int32_t offered, uint32_t modifiers)
{
    return drag_actions_for_modifiers(offered, modifiers);
}

static void test_ctrl_copies_and_shift_moves(void)
{
    CHECK_INT_EQ(COPY, narrowed(COPY | MOVE, WM_MOD_CTRL));
    CHECK_INT_EQ(MOVE, narrowed(COPY | MOVE, WM_MOD_SHIFT));
    CHECK_INT_EQ(COPY | MOVE, narrowed(COPY | MOVE, WM_MOD_CTRL | WM_MOD_SHIFT));
    CHECK_INT_EQ(COPY | MOVE, narrowed(COPY | MOVE, WM_MOD_ALT));
    CHECK_INT_EQ(MOVE, narrowed(MOVE, WM_MOD_CTRL));
}

static void test_modifiers_keep_ask_drags(void)
{
    CHECK_INT_EQ(COPY | MOVE | ASK, narrowed(COPY | MOVE | ASK, WM_MOD_CTRL));
    CHECK_INT_EQ(-1, narrowed(-1, WM_MOD_CTRL));
}

static uint32_t chosen(uint32_t offer, uint32_t preferred, int32_t source)
{
    drag_choice choice = {true, offer, preferred, source};
    return drag_actions_choose(&choice);
}

static void test_choice_order(void)
{
    CHECK_INT_EQ(COPY, (int32_t) chosen(COPY | MOVE, MOVE, COPY));
    CHECK_INT_EQ(MOVE, (int32_t) chosen(COPY | MOVE, MOVE, COPY | MOVE));
    CHECK_INT_EQ(COPY, (int32_t) chosen(COPY | MOVE, 0, COPY | MOVE));
    CHECK_INT_EQ(0, (int32_t) chosen(ASK, 0, COPY | MOVE));
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
        {"ctrl_copies_and_shift_moves", test_ctrl_copies_and_shift_moves},
        {"modifiers_keep_ask_drags", test_modifiers_keep_ask_drags},
        {"choice_order", test_choice_order},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
