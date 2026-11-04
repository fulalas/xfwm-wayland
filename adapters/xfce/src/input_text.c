#include "input_text.h"

#include <glib.h>
#include <stdbool.h>

char *input_device_property_prefix(const char *device_name)
{
    GString *prefix = g_string_new("/");

    for (const char *c = device_name; *c; c++) {
        if (g_ascii_isalnum(*c) || *c == '-' || *c == '_') {
            g_string_append_c(prefix, *c);
        } else if (*c == ' ') {
            g_string_append_c(prefix, '_');
        }
    }
    g_string_append_c(prefix, '/');
    return g_string_free(prefix, FALSE);
}

void input_join_options(const char *const *parts, char *joined, size_t size)
{
    joined[0] = '\0';
    for (size_t i = 0; parts[i]; i++) {
        if (*parts[i] == '\0') {
            continue;
        }
        if (joined[0] != '\0') {
            g_strlcat(joined, ",", size);
        }
        g_strlcat(joined, parts[i], size);
    }
}

optional_bool input_bool_from_text(const char *text)
{
    optional_double number;

    if (!text) {
        return (optional_bool) { .is_set = false };
    }
    if (g_ascii_strcasecmp(text, "true") == 0) {
        return (optional_bool) { .is_set = true, .value = true };
    }
    if (g_ascii_strcasecmp(text, "false") == 0) {
        return (optional_bool) { .is_set = true, .value = false };
    }
    number = input_double_from_text(text);
    return (optional_bool) { .is_set = number.is_set, .value = number.value != 0.0 };
}

optional_double input_double_from_text(const char *text)
{
    char *end = NULL;
    double value;

    if (!text || *text == '\0') {
        return (optional_double) { .is_set = false };
    }
    value = g_ascii_strtod(text, &end);
    if (*end != '\0') {
        return (optional_double) { .is_set = false };
    }
    return (optional_double) { .is_set = true, .value = value };
}
