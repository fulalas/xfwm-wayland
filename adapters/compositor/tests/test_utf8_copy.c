#include <string.h>

#include "check.h"
#include "utf8_copy.h"

static void test_short_text_is_copied_whole(void)
{
    char dest[8];
    utf8_copy(dest, sizeof(dest), "abc");
    CHECK(strcmp(dest, "abc") == 0);
}

static void test_null_source_gives_empty_text(void)
{
    char dest[8] = "xyz";
    utf8_copy(dest, sizeof(dest), NULL);
    CHECK(strcmp(dest, "") == 0);
}

static void test_cut_keeps_whole_ascii_prefix(void)
{
    char dest[4];
    utf8_copy(dest, sizeof(dest), "abcdef");
    CHECK(strcmp(dest, "abc") == 0);
}

static void test_cut_drops_a_split_two_byte_character(void)
{
    char dest[4];
    utf8_copy(dest, sizeof(dest), "ab\xc3\xa9");
    CHECK(strcmp(dest, "ab") == 0);
}

static void test_cut_keeps_a_complete_multibyte_character(void)
{
    char dest[5];
    utf8_copy(dest, sizeof(dest), "a\xe2\x82\xac" "b");
    CHECK(strcmp(dest, "a\xe2\x82\xac") == 0);
}

static void test_cut_drops_a_split_four_byte_character(void)
{
    char dest[4];
    utf8_copy(dest, sizeof(dest), "\xf0\x9f\x98\x80");
    CHECK(strcmp(dest, "") == 0);
}

int main(void)
{
    const check_case cases[] = {
        {"short_text_is_copied_whole", test_short_text_is_copied_whole},
        {"null_source_gives_empty_text", test_null_source_gives_empty_text},
        {"cut_keeps_whole_ascii_prefix", test_cut_keeps_whole_ascii_prefix},
        {"cut_drops_a_split_two_byte_character", test_cut_drops_a_split_two_byte_character},
        {"cut_keeps_a_complete_multibyte_character",
         test_cut_keeps_a_complete_multibyte_character},
        {"cut_drops_a_split_four_byte_character", test_cut_drops_a_split_four_byte_character},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
