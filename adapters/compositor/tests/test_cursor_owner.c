#include "check.h"
#include "cursor_owner.h"

static const int source_client = 1;
static const int target_client = 2;

static void test_focused_client_sets_cursor_outside_drag(void)
{
    cursor_request request = {.requester = &target_client, .pointer_focus = &target_client,
                              .client_mode = true};
    CHECK(cursor_owner_accepts(&request));
}

static void test_unfocused_client_is_refused_outside_drag(void)
{
    cursor_request request = {.requester = &source_client, .pointer_focus = &target_client,
                              .client_mode = true};
    CHECK(!cursor_owner_accepts(&request));
}

static void test_core_cursor_refuses_clients(void)
{
    cursor_request request = {.requester = &target_client, .pointer_focus = &target_client};
    CHECK(!cursor_owner_accepts(&request));
}

static void test_drag_source_sets_cursor_without_pointer_focus(void)
{
    cursor_request request = {.requester = &source_client, .drag_source = &source_client,
                              .drag_active = true};
    CHECK(cursor_owner_accepts(&request));
}

static void test_drag_target_is_refused_during_drag(void)
{
    cursor_request request = {.requester = &target_client, .pointer_focus = &target_client,
                              .drag_source = &source_client, .drag_active = true,
                              .client_mode = true};
    CHECK(!cursor_owner_accepts(&request));
}

static void test_drag_without_source_client_refuses_all(void)
{
    cursor_request request = {.requester = &source_client, .drag_active = true};
    CHECK(!cursor_owner_accepts(&request));
}

int main(void)
{
    const check_case cases[] = {
        {"focused_client_sets_cursor_outside_drag", test_focused_client_sets_cursor_outside_drag},
        {"unfocused_client_is_refused_outside_drag",
         test_unfocused_client_is_refused_outside_drag},
        {"core_cursor_refuses_clients", test_core_cursor_refuses_clients},
        {"drag_source_sets_cursor_without_pointer_focus",
         test_drag_source_sets_cursor_without_pointer_focus},
        {"drag_target_is_refused_during_drag", test_drag_target_is_refused_during_drag},
        {"drag_without_source_client_refuses_all", test_drag_without_source_client_refuses_all},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
