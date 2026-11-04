#include "protocol_io.h"

#include <gio/gio.h>
#include <gio/gunixinputstream.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "protocol_text.h"

enum {
    ICON_FIELD_COUNT = 5,
    DECIMAL = 10,
};

typedef struct listener {
    request_handler handler;
    GDataInputStream *input;
} listener;

static void read_next(listener *listener);
static void on_line(GObject *source, GAsyncResult *result, void *data);
static void handle_line(listener *listener, const char *line);
static void write_field(const char *text);

void protocol_listen(const request_handler *handler)
{
    listener *listener = g_new0(struct listener, 1);
    GInputStream *stdin_stream = g_unix_input_stream_new(STDIN_FILENO, FALSE);

    listener->handler = *handler;
    listener->input = g_data_input_stream_new(stdin_stream);
    g_object_unref(stdin_stream);
    read_next(listener);
}

void protocol_write(const char *verb, const char *first, const char *second)
{
    fputs(verb, stdout);
    if (first) {
        write_field(first);
    }
    if (second) {
        write_field(second);
    }
    fputc('\n', stdout);
    fflush(stdout);
}

static void read_next(listener *listener)
{
    g_data_input_stream_read_line_async(listener->input, G_PRIORITY_DEFAULT, NULL, on_line,
                                        listener);
}

static void on_line(GObject *source, GAsyncResult *result, void *data)
{
    listener *listener = data;
    char *line = g_data_input_stream_read_line_finish(G_DATA_INPUT_STREAM(source), result, NULL,
                                                      NULL);

    if (!line) {
        listener->handler.closed(listener->handler.self);
        return;
    }
    handle_line(listener, line);
    g_free(line);
    read_next(listener);
}

static void handle_line(listener *listener, const char *line)
{
    char **fields = g_strsplit(line, "\t", ICON_FIELD_COUNT + 1);
    char *app_id;
    char *icon_name;
    icon_query query;

    if (g_strv_length(fields) != ICON_FIELD_COUNT || strcmp(fields[0], "icon") != 0) {
        g_strfreev(fields);
        return;
    }
    app_id = protocol_unescape(fields[3]);
    icon_name = protocol_unescape(fields[4]);
    query = (icon_query) {
        .id = (unsigned) strtoul(fields[1], NULL, DECIMAL),
        .size = atoi(fields[2]),
        .app_id = app_id,
        .icon_name = icon_name,
    };
    listener->handler.icon(listener->handler.self, &query);
    g_free(app_id);
    g_free(icon_name);
    g_strfreev(fields);
}

static void write_field(const char *text)
{
    char *escaped = protocol_escape(text);

    fputc('\t', stdout);
    fputs(escaped, stdout);
    g_free(escaped);
}
