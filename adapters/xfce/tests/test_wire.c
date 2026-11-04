#include <glib.h>
#include <string.h>

#include "check.h"
#include "helper_protocol.h"
#include "protocol_text.h"

enum {
    ICON_FIELD_COUNT = 5,
    APP_ID_FIELD = 3,
    ICON_NAME_FIELD = 4,
};

static const char *const samples[] = {
    "",
    "plain",
    "tab\there",
    "line\nbreak",
    "back\\slash",
    "\\t not a tab",
    "\\n not a newline",
    "ends with \\",
    "\\",
    "\t\n\\\t\\\n",
};

static char *helper_reads_icon_field(const char *app_id, int field)
{
    icon_request request = { .app_id = app_id, .icon_name = app_id, .size = 16 };
    char *line = helper_icon_request_format(1, &request);
    char **fields;
    char *value;

    line[strlen(line) - 1] = '\0';
    fields = g_strsplit(line, "\t", ICON_FIELD_COUNT + 1);
    CHECK_INT_EQ(ICON_FIELD_COUNT, g_strv_length(fields));
    value = protocol_unescape(fields[field]);
    g_strfreev(fields);
    g_free(line);
    return value;
}

static void test_xfce_escape_helper_unescape(void)
{
    for (size_t i = 0; i < G_N_ELEMENTS(samples); i++) {
        char *app_id = helper_reads_icon_field(samples[i], APP_ID_FIELD);
        char *icon_name = helper_reads_icon_field(samples[i], ICON_NAME_FIELD);
        CHECK(strcmp(app_id, samples[i]) == 0);
        CHECK(strcmp(icon_name, samples[i]) == 0);
        g_free(app_id);
        g_free(icon_name);
    }
}

static void test_helper_escape_xfce_unescape(void)
{
    helper_message message;

    for (size_t i = 0; i < G_N_ELEMENTS(samples); i++) {
        char *escaped = protocol_escape(samples[i]);
        char *line = g_strconcat("font\t", escaped, NULL);
        CHECK(helper_message_parse(line, &message));
        CHECK(strcmp(message.text, samples[i]) == 0);
        g_free(line);
        g_free(escaped);
    }
}

int main(void)
{
    static const check_case cases[] = {
        { "xfce_escape_helper_unescape", test_xfce_escape_helper_unescape },
        { "helper_escape_xfce_unescape", test_helper_escape_xfce_unescape },
    };

    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
