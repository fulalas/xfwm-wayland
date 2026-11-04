#include <stdlib.h>
#include <string.h>

#include "check.h"
#include "session_record.h"
#include "wm/desktop.h"

static session_record sample(const char *name)
{
    return (session_record){
        .name = (char *) name,
        .geometry = {-20, 40, 800, 600},
        .states = WM_STATE_MAXIMIZED | WM_STATE_ABOVE,
        .has_workspace = true,
        .workspace = 2,
        .seen = 1700000000,
    };
}

static session_record round_trip(const session_record *record, bool *parsed)
{
    char *line = session_record_format(record);
    session_record copy;
    *parsed = line != NULL && session_record_parse(line, &copy);
    free(line);
    return copy;
}

static void test_record_survives_round_trip(void)
{
    session_record original = sample("main-window");
    bool parsed = false;
    session_record copy = round_trip(&original, &parsed);
    CHECK(parsed);
    CHECK(strcmp("main-window", copy.name) == 0);
    CHECK_RECT_EQ(original.geometry, copy.geometry);
    CHECK_INT_EQ(original.states, copy.states);
    CHECK(copy.has_workspace);
    CHECK_INT_EQ(2, copy.workspace);
    CHECK_INT_EQ(1700000000, copy.seen);
    session_record_finish(&copy);
}

static void test_name_with_spaces_and_newlines_survives(void)
{
    session_record original = sample("a b\nc=d%e\t\xc3\xa9");
    bool parsed = false;
    session_record copy = round_trip(&original, &parsed);
    CHECK(parsed);
    CHECK(strcmp(original.name, copy.name) == 0);
    session_record_finish(&copy);
}

static void test_record_without_workspace(void)
{
    session_record original = sample("w");
    original.has_workspace = false;
    original.states = WM_STATE_STICKY;
    bool parsed = false;
    session_record copy = round_trip(&original, &parsed);
    CHECK(parsed);
    CHECK(!copy.has_workspace);
    CHECK_INT_EQ(WM_STATE_STICKY, copy.states);
    session_record_finish(&copy);
}

static void test_unknown_keys_and_states_are_ignored(void)
{
    session_record record;
    CHECK(session_record_parse("future=1 name=w x=1 y=2 w=3 h=4 states=above,wobbly", &record));
    CHECK_INT_EQ(WM_STATE_ABOVE, record.states);
    CHECK_RECT_EQ(((wm_rect) {1, 2, 3, 4}), record.geometry);
    session_record_finish(&record);
}

static void test_broken_lines_are_rejected(void)
{
    session_record record;
    CHECK(!session_record_parse("x=1 y=2 w=3 h=4", &record));
    CHECK(!session_record_parse("name=w x=1 y=2 w=0 h=4", &record));
    CHECK(!session_record_parse("name=bad%zz x=1 y=2 w=3 h=4", &record));
    CHECK(!session_record_parse("name=nul%00 x=1 y=2 w=3 h=4", &record));
    CHECK(!session_record_parse("name=cut% x=1 y=2 w=3 h=4", &record));
    CHECK(!session_record_parse("", &record));
}

static void test_session_ids_are_lowercase_hex(void)
{
    CHECK(session_id_is_valid("0123456789abcdef0123456789abcdef"));
    CHECK(!session_id_is_valid("0123456789ABCDEF0123456789abcdef"));
    CHECK(!session_id_is_valid("../../../../../../etc/passwd/abcd"));
    CHECK(!session_id_is_valid("0123456789abcdef"));
    CHECK(!session_id_is_valid(NULL));
}

static void test_sessions_expire_after_ninety_days(void)
{
    int64_t day = 86400;
    CHECK(!session_is_expired(0, 90 * day));
    CHECK(session_is_expired(0, 90 * day + 1));
}

int main(void)
{
    static const check_case cases[] = {
        {"record_survives_round_trip", test_record_survives_round_trip},
        {"name_with_spaces_and_newlines_survives", test_name_with_spaces_and_newlines_survives},
        {"record_without_workspace", test_record_without_workspace},
        {"unknown_keys_and_states_are_ignored", test_unknown_keys_and_states_are_ignored},
        {"broken_lines_are_rejected", test_broken_lines_are_rejected},
        {"session_ids_are_lowercase_hex", test_session_ids_are_lowercase_hex},
        {"sessions_expire_after_ninety_days", test_sessions_expire_after_ninety_days},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
