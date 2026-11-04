#include "protocol_text.h"

#include <glib.h>

char *protocol_escape(const char *text)
{
    GString *result = g_string_new(NULL);

    for (const char *c = text; *c; c++) {
        if (*c == '\\' || *c == '\t' || *c == '\n') {
            g_string_append_c(result, '\\');
        }
        g_string_append_c(result, *c == '\t' ? 't' : *c == '\n' ? 'n' : *c);
    }
    return g_string_free(result, FALSE);
}

char *protocol_unescape(const char *text)
{
    GString *result = g_string_new(NULL);

    for (const char *c = text; *c; c++) {
        if (*c != '\\' || c[1] == '\0') {
            g_string_append_c(result, *c);
            continue;
        }
        c++;
        g_string_append_c(result, *c == 't' ? '\t' : *c == 'n' ? '\n' : *c);
    }
    return g_string_free(result, FALSE);
}
