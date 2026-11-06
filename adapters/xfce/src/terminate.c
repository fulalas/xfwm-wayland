#include "terminate.h"

#include <gio/gio.h>

#include "helper_path.h"
#include "helper_protocol.h"

struct terminate_dialogs {
    GList *jobs;
};

typedef struct terminate_job {
    terminate_dialogs *owner;
    void (*done)(void *context, bool terminate);
    void *context;
    GCancellable *cancellable;
    GSubprocess *process;
} terminate_job;

static GSubprocess *spawn_dialog(const char *title);
static void on_answer(GObject *source, GAsyncResult *result, void *data);
static gboolean on_spawn_failed(void *data);
static void finish_job(terminate_job *job, bool terminate);
static void cancel_job(terminate_job *job);

terminate_dialogs *terminate_dialogs_create(void)
{
    return g_new0(terminate_dialogs, 1);
}

void terminate_dialogs_destroy(terminate_dialogs *dialogs)
{
    for (GList *link = dialogs->jobs; link; link = link->next) {
        terminate_job *job = link->data;
        job->owner = NULL;
        cancel_job(job);
    }
    g_list_free(dialogs->jobs);
    g_free(dialogs);
}

void terminate_dialogs_ask(terminate_dialogs *dialogs, const terminate_request *request)
{
    terminate_job *job = g_new0(terminate_job, 1);
    GSubprocess *process = spawn_dialog(request->title ? request->title : "");

    *job = (terminate_job) {
        .owner = dialogs,
        .done = request->done,
        .context = request->context,
        .cancellable = g_cancellable_new(),
        .process = process,
    };
    dialogs->jobs = g_list_prepend(dialogs->jobs, job);
    if (!process) {
        g_idle_add(on_spawn_failed, job);
        return;
    }
    g_subprocess_communicate_utf8_async(process, NULL, job->cancellable, on_answer, job);
}

void terminate_dialogs_cancel(terminate_dialogs *dialogs, void *context)
{
    for (GList *link = dialogs->jobs; link; link = link->next) {
        terminate_job *job = link->data;
        if (job->context == context) {
            cancel_job(job);
        }
    }
}

static GSubprocess *spawn_dialog(const char *title)
{
    char *path = helper_path_find();
    GError *error = NULL;
    GSubprocess *process = NULL;

    if (path) {
        process = g_subprocess_new(G_SUBPROCESS_FLAGS_STDOUT_PIPE, &error, path, "--terminate",
                                   title, NULL);
    }
    if (!process) {
        g_warning("Cannot show the terminate dialog: %s",
                  error ? error->message : "helper not found");
        g_clear_error(&error);
    }
    g_free(path);
    return process;
}

static void on_answer(GObject *source, GAsyncResult *result, void *data)
{
    terminate_job *job = data;
    char *output = NULL;
    GError *error = NULL;

    g_subprocess_communicate_utf8_finish(G_SUBPROCESS(source), result, &output, NULL, &error);
    if (error && !g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
        g_warning("Cannot read the terminate dialog answer: %s", error->message);
    }
    g_clear_error(&error);
    finish_job(job, helper_terminate_answer(output));
    g_free(output);
}

static gboolean on_spawn_failed(void *data)
{
    finish_job(data, false);
    return G_SOURCE_REMOVE;
}

static void finish_job(terminate_job *job, bool terminate)
{
    if (job->owner) {
        job->owner->jobs = g_list_remove(job->owner->jobs, job);
    }
    if (!g_cancellable_is_cancelled(job->cancellable)) {
        job->done(job->context, terminate);
    }
    g_clear_object(&job->process);
    g_object_unref(job->cancellable);
    g_free(job);
}

static void cancel_job(terminate_job *job)
{
    g_cancellable_cancel(job->cancellable);
    if (job->process) {
        g_subprocess_force_exit(job->process);
    }
}
