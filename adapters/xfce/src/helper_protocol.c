#include "helper_protocol.h"
#include "protocol_text.h"

#include <glib.h>
#include <stdlib.h>
#include <string.h>

enum {
    FIELD_MAX = 4,
    COLOR_HEX_DIGITS = 8,
    COLOR_CHANNEL_MAX = 255,
    RED_SHIFT = 24,
    GREEN_SHIFT = 16,
    BLUE_SHIFT = 8,
    ALPHA_SHIFT = 0,
    DECIMAL = 10,
    HEXADECIMAL = 16,
};

typedef struct message_shape {
    const char *verb;
    helper_message_kind kind;
    unsigned field_count;
} message_shape;

static const message_shape message_shapes[] = {
    { "style-begin", HELPER_MESSAGE_STYLE_BEGIN, 1 },
    { "symbol", HELPER_MESSAGE_SYMBOL, 3 },
    { "color", HELPER_MESSAGE_COLOR, 3 },
    { "font", HELPER_MESSAGE_FONT, 2 },
    { "style-end", HELPER_MESSAGE_STYLE_END, 1 },
    { "icon", HELPER_MESSAGE_ICON, 3 },
};

static bool fill_message(char **fields, helper_message *message);
static bool fill_fields(const message_shape *shape, char **fields, helper_message *message);
static bool parse_id(const char *text, unsigned *id);
static void append_escaped(GString *line, const char *text);
static bool is_hex_color(const char *digits);
static double color_channel(guint32 rgba, unsigned shift);

bool helper_message_parse(const char *line, helper_message *message)
{
    char **fields = g_strsplit(line, "\t", FIELD_MAX + 1);
    bool parsed = fill_message(fields, message);

    g_strfreev(fields);
    return parsed;
}

bool helper_color_parse(const char *text, xfce_color *color)
{
    guint32 rgba;

    if (text[0] != '#' || !is_hex_color(text + 1)) {
        return false;
    }
    rgba = (guint32) strtoul(text + 1, NULL, HEXADECIMAL);
    *color = (xfce_color) {
        .red = color_channel(rgba, RED_SHIFT),
        .green = color_channel(rgba, GREEN_SHIFT),
        .blue = color_channel(rgba, BLUE_SHIFT),
        .alpha = color_channel(rgba, ALPHA_SHIFT),
    };
    return true;
}

char *helper_icon_request_format(unsigned id, const icon_request *request)
{
    GString *line = g_string_new(NULL);

    g_string_printf(line, "icon\t%u\t%d\t", id, request->size);
    append_escaped(line, request->app_id ? request->app_id : "");
    g_string_append_c(line, '\t');
    append_escaped(line, request->icon_name ? request->icon_name : "");
    g_string_append_c(line, '\n');
    return g_string_free(line, FALSE);
}

bool helper_terminate_answer(const char *output)
{
    return output && (strcmp(output, "yes") == 0 || g_str_has_prefix(output, "yes\n"));
}

static bool fill_message(char **fields, helper_message *message)
{
    unsigned count = g_strv_length(fields);

    for (size_t i = 0; i < G_N_ELEMENTS(message_shapes); i++) {
        const message_shape *shape = &message_shapes[i];
        if (count > 0 && strcmp(fields[0], shape->verb) == 0) {
            return count == shape->field_count && fill_fields(shape, fields, message);
        }
    }
    return false;
}

static bool fill_fields(const message_shape *shape, char **fields, helper_message *message)
{
    char *last = shape->field_count > 1 ? protocol_unescape(fields[shape->field_count - 1]) : NULL;
    bool valid = true;

    *message = (helper_message) { .kind = shape->kind };
    if (last) {
        g_strlcpy(message->text, last, sizeof(message->text));
    }
    if (shape->kind == HELPER_MESSAGE_ICON) {
        valid = parse_id(fields[1], &message->id);
    } else if (shape->field_count == 3) {
        g_strlcpy(message->name, fields[1], sizeof(message->name));
    }
    g_free(last);
    return valid;
}

static bool parse_id(const char *text, unsigned *id)
{
    char *end = NULL;
    unsigned long value = strtoul(text, &end, DECIMAL);

    if (*text == '\0' || *end != '\0' || value > G_MAXUINT) {
        return false;
    }
    *id = (unsigned) value;
    return true;
}

static void append_escaped(GString *line, const char *text)
{
    char *escaped = protocol_escape(text);
    g_string_append(line, escaped);
    g_free(escaped);
}

static bool is_hex_color(const char *digits)
{
    size_t length = strlen(digits);

    for (size_t i = 0; i < length; i++) {
        if (!g_ascii_isxdigit(digits[i])) {
            return false;
        }
    }
    return length == COLOR_HEX_DIGITS;
}

static double color_channel(guint32 rgba, unsigned shift)
{
    return (double) ((rgba >> shift) & COLOR_CHANNEL_MAX) / COLOR_CHANNEL_MAX;
}
