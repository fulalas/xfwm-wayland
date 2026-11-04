#include "helper_session.h"

#include <gio/gio.h>
#include <string.h>

struct helper_session {
    session_events events;
    GSubprocess *process;
    GDataInputStream *input;
    GOutputStream *output;
    GCancellable *cancellable;
    GQueue outbox;
    GBytes *writing;
};

static void read_next(helper_session *session);
static void on_line(GObject *source, GAsyncResult *result, void *data);
static void write_next(helper_session *session);
static void on_written(GObject *source, GAsyncResult *result, void *data);
static void on_exited(GObject *source, GAsyncResult *result, void *data);
static bool is_cancelled(const GError *error);

helper_session *helper_session_spawn(const char *path, const session_events *events)
{
    GError *error = NULL;
    GSubprocessFlags flags = G_SUBPROCESS_FLAGS_STDIN_PIPE | G_SUBPROCESS_FLAGS_STDOUT_PIPE;
    GSubprocess *process = g_subprocess_new(flags, &error, path, NULL);
    helper_session *session;

    if (!process) {
        g_warning("Cannot start the GTK helper %s: %s", path, error->message);
        g_error_free(error);
        return NULL;
    }
    session = g_new0(helper_session, 1);
    session->events = *events;
    session->process = process;
    session->input = g_data_input_stream_new(g_subprocess_get_stdout_pipe(process));
    session->output = g_object_ref(g_subprocess_get_stdin_pipe(process));
    session->cancellable = g_cancellable_new();
    g_queue_init(&session->outbox);
    g_subprocess_wait_async(process, session->cancellable, on_exited, session);
    read_next(session);
    return session;
}

void helper_session_destroy(helper_session *session)
{
    g_cancellable_cancel(session->cancellable);
    g_subprocess_force_exit(session->process);
    g_queue_clear_full(&session->outbox, (GDestroyNotify) g_bytes_unref);
    g_clear_pointer(&session->writing, g_bytes_unref);
    g_object_unref(session->cancellable);
    g_object_unref(session->output);
    g_object_unref(session->input);
    g_object_unref(session->process);
    g_free(session);
}

void helper_session_send(helper_session *session, const char *line)
{
    g_queue_push_tail(&session->outbox, g_bytes_new(line, strlen(line)));
    write_next(session);
}

static void read_next(helper_session *session)
{
    g_data_input_stream_read_line_async(session->input, G_PRIORITY_DEFAULT,
                                        session->cancellable, on_line, session);
}

static void on_line(GObject *source, GAsyncResult *result, void *data)
{
    GError *error = NULL;
    char *line = g_data_input_stream_read_line_finish(G_DATA_INPUT_STREAM(source), result, NULL,
                                                      &error);
    helper_session *session = data;

    if (is_cancelled(error) || !line) {
        g_clear_error(&error);
        return;
    }
    session->events.line(session->events.self, line);
    g_free(line);
    read_next(session);
}

static void write_next(helper_session *session)
{
    gsize size;
    const void *bytes;

    if (session->writing || g_queue_is_empty(&session->outbox)) {
        return;
    }
    session->writing = g_queue_pop_head(&session->outbox);
    bytes = g_bytes_get_data(session->writing, &size);
    g_output_stream_write_all_async(session->output, bytes, size, G_PRIORITY_DEFAULT,
                                    session->cancellable, on_written, session);
}

static void on_written(GObject *source, GAsyncResult *result, void *data)
{
    GError *error = NULL;
    bool written = g_output_stream_write_all_finish(G_OUTPUT_STREAM(source), result, NULL, &error);
    helper_session *session = data;

    if (is_cancelled(error)) {
        g_error_free(error);
        return;
    }
    g_clear_pointer(&session->writing, g_bytes_unref);
    if (!written) {
        g_warning("Cannot write to the GTK helper: %s", error->message);
        g_error_free(error);
        return;
    }
    write_next(session);
}

static void on_exited(GObject *source, GAsyncResult *result, void *data)
{
    GError *error = NULL;
    bool waited = g_subprocess_wait_finish(G_SUBPROCESS(source), result, &error);
    helper_session *session = data;

    if (is_cancelled(error)) {
        g_error_free(error);
        return;
    }
    if (!waited) {
        g_warning("Cannot wait for the GTK helper: %s", error->message);
        g_error_free(error);
    }
    session->events.exited(session->events.self);
}

static bool is_cancelled(const GError *error)
{
    return g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
}
