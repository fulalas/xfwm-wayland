#include <string.h>

#include <glib.h>

#include "check.h"
#include "fixture.h"
#include "themerc.h"

static themerc *parse(const char *content)
{
    char *dir = fixture_dir_create();
    char *path = fixture_write(dir, "themerc", content);
    themerc *rc = appearance_themerc_create();

    CHECK(appearance_themerc_parse_file(rc, path));
    g_free(path);
    fixture_dir_remove(dir);
    return rc;
}

static bool value_is(const themerc *rc, const char *key, const char *expected)
{
    const char *value = appearance_themerc_get(rc, key);

    return value != NULL && strcmp(value, expected) == 0;
}

static void test_key_value_lines(void)
{
    themerc *rc = parse("button_offset=2\nfull_width_title=true\n");

    CHECK(value_is(rc, "button_offset", "2"));
    CHECK(value_is(rc, "full_width_title", "true"));
    appearance_themerc_destroy(rc);
}

static void test_keys_are_case_insensitive(void)
{
    themerc *rc = parse("Active_Text_Color=#ffffff\n");

    CHECK(value_is(rc, "active_text_color", "#ffffff"));
    CHECK(value_is(rc, "ACTIVE_TEXT_COLOR", "#ffffff"));
    appearance_themerc_destroy(rc);
}

static void test_value_runs_to_end_of_line(void)
{
    themerc *rc = parse("title_font=Sans Bold 9 = x \nlast=no newline");

    CHECK(value_is(rc, "title_font", "Sans Bold 9 = x "));
    CHECK(value_is(rc, "last", "no newline"));
    appearance_themerc_destroy(rc);
}

static void test_lines_without_value_are_ignored(void)
{
    themerc *rc = parse("# comment\nempty=\nkept=1\n");

    CHECK(appearance_themerc_get(rc, "# comment\n") == NULL);
    CHECK(appearance_themerc_get(rc, "empty") == NULL);
    CHECK(value_is(rc, "kept", "1"));
    appearance_themerc_destroy(rc);
}

static void test_overly_long_lines_are_skipped(void)
{
    GString *content = g_string_new("long=");
    for (int i = 0; i < 2000; i++) {
        g_string_append_c(content, 'x');
    }
    g_string_append(content, "\nafter=ok\n");
    themerc *rc = parse(content->str);

    CHECK(appearance_themerc_get(rc, "long") == NULL);
    CHECK(value_is(rc, "after", "ok"));
    appearance_themerc_destroy(rc);
    g_string_free(content, TRUE);
}

static void test_later_values_override(void)
{
    themerc *rc = parse("button_spacing=1\nBUTTON_SPACING=3\n");

    CHECK(value_is(rc, "button_spacing", "3"));
    appearance_themerc_destroy(rc);
}

static void test_missing_file_fails(void)
{
    themerc *rc = appearance_themerc_create();

    CHECK(!appearance_themerc_parse_file(rc, "/tmp/xfwm-appearance-missing/themerc"));
    appearance_themerc_destroy(rc);
}

int main(void)
{
    static const check_case cases[] = {
        { "key_value_lines", test_key_value_lines },
        { "keys_are_case_insensitive", test_keys_are_case_insensitive },
        { "value_runs_to_end_of_line", test_value_runs_to_end_of_line },
        { "lines_without_value_are_ignored", test_lines_without_value_are_ignored },
        { "overly_long_lines_are_skipped", test_overly_long_lines_are_skipped },
        { "later_values_override", test_later_values_override },
        { "missing_file_fails", test_missing_file_fails },
    };

    return check_run(cases, sizeof cases / sizeof cases[0]);
}
