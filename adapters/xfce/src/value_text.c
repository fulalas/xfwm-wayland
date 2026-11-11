#include "value_text.h"

static char *number_to_text(const GValue *value);
static char *array_to_text(const GPtrArray *values);

char *value_text_of(const GValue *value)
{
    if (G_VALUE_HOLDS_STRING(value)) {
        return g_value_dup_string(value);
    }
    if (G_VALUE_HOLDS_BOOLEAN(value)) {
        return g_strdup(g_value_get_boolean(value) ? "true" : "false");
    }
    if (G_VALUE_HOLDS(value, G_TYPE_PTR_ARRAY)) {
        return array_to_text(g_value_get_boxed(value));
    }
    return number_to_text(value);
}

static char *number_to_text(const GValue *value)
{
    GValue number = G_VALUE_INIT;
    char buffer[G_ASCII_DTOSTR_BUF_SIZE];

    if (!g_value_type_transformable(G_VALUE_TYPE(value), G_TYPE_DOUBLE)) {
        return NULL;
    }
    g_value_init(&number, G_TYPE_DOUBLE);
    g_value_transform(value, &number);
    g_ascii_dtostr(buffer, sizeof(buffer), g_value_get_double(&number));
    return g_strdup(buffer);
}

static char *array_to_text(const GPtrArray *values)
{
    GString *text = g_string_new(NULL);

    for (guint i = 0; values && i < values->len; i++) {
        char *item = value_text_of(g_ptr_array_index(values, i));

        if (i > 0) {
            g_string_append_c(text, ',');
        }
        g_string_append(text, item ? item : "");
        g_free(item);
    }
    return g_string_free(text, FALSE);
}
