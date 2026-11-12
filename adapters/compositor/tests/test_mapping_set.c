#include "check.h"
#include "server/mapping_set.h"

static void test_added_mapping_is_found(void)
{
    mapping_set set = {0};
    mapping buffer = {0x1000, 0x4000};
    mapping_set_add(&set, &buffer);
    CHECK(mapping_set_has(&set, &buffer));
}

static void test_other_start_or_length_is_not_found(void)
{
    mapping_set set = {0};
    mapping buffer = {0x1000, 0x4000};
    mapping moved = {0x2000, 0x4000};
    mapping resized = {0x1000, 0x8000};
    mapping_set_add(&set, &buffer);
    CHECK(!mapping_set_has(&set, &moved));
    CHECK(!mapping_set_has(&set, &resized));
}

static void test_clear_forgets_everything(void)
{
    mapping_set set = {0};
    mapping buffer = {0x1000, 0x4000};
    mapping_set_add(&set, &buffer);
    mapping_set_clear(&set);
    CHECK(!mapping_set_has(&set, &buffer));
}

static void test_full_set_ignores_more(void)
{
    mapping_set set = {0};
    for (size_t i = 0; i <= MAPPING_SET_MAX; i++) {
        mapping item = {0x1000 * (i + 1), 0x1000};
        mapping_set_add(&set, &item);
    }
    mapping first = {0x1000, 0x1000};
    mapping last = {0x1000 * (MAPPING_SET_MAX + 1), 0x1000};
    CHECK_INT_EQ(MAPPING_SET_MAX, (int) set.count);
    CHECK(mapping_set_has(&set, &first));
    CHECK(!mapping_set_has(&set, &last));
}

int main(void)
{
    const check_case cases[] = {
        {"added_mapping_is_found", test_added_mapping_is_found},
        {"other_start_or_length_is_not_found", test_other_start_or_length_is_not_found},
        {"clear_forgets_everything", test_clear_forgets_everything},
        {"full_set_ignores_more", test_full_set_ignores_more},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
