#include <stdlib.h>
#include "session_launch.h"

#include <glib.h>
#include <stdbool.h>
#include <sys/wait.h>

#include "session_export.h"

struct session_launch {
    session_launch_listener listener;
    char *command;
    GSource *watch;
    int exit_code;
};

static void start_watch(session_launch *launch, GSource *source, GSourceFunc callback);
static void forget_watch(session_launch *launch);
static gboolean on_start(gpointer data);
static void run_activation_update(session_launch *launch);
static void on_activation_updated(GPid pid, gint status, gpointer data);
static void run_session(session_launch *launch);
static bool spawn_watched(session_launch *launch, char **argv, GChildWatchFunc on_exit);
static void on_session_exited(GPid pid, gint status, gpointer data);
static void report_exit(const char *command, gint status);
static int exit_code_of(gint status);
static void notify_ended(session_launch *launch);

session_launch *session_launch_start(const char *command, const session_launch_listener *listener)
{
    session_launch *launch = g_new0(session_launch, 1);

    launch->listener = *listener;
    launch->command = g_strdup(command);
    start_watch(launch, g_idle_source_new(), on_start);
    return launch;
}

void session_launch_destroy(session_launch *launch)
{
    if (!launch) {
        return;
    }
    if (launch->watch) {
        g_source_destroy(launch->watch);
        forget_watch(launch);
    }
    g_free(launch->command);
    g_free(launch);
}

static void start_watch(session_launch *launch, GSource *source, GSourceFunc callback)
{
    g_source_set_callback(source, callback, launch, NULL);
    g_source_attach(source, g_main_context_get_thread_default());
    launch->watch = source;
}

static void forget_watch(session_launch *launch)
{
    g_clear_pointer(&launch->watch, g_source_unref);
}

static gboolean on_start(gpointer data)
{
    session_launch *launch = data;

    forget_watch(launch);
    run_activation_update(launch);
    return G_SOURCE_REMOVE;
}

static void run_activation_update(session_launch *launch)
{
    char **argv = session_export_activation_argv();
    bool started = argv && spawn_watched(launch, argv, on_activation_updated);

    g_strfreev(argv);
    if (!started) {
        run_session(launch);
    }
}

static void on_activation_updated(GPid pid, gint status, gpointer data)
{
    session_launch *launch = data;

    (void) status;
    g_spawn_close_pid(pid);
    forget_watch(launch);
    run_session(launch);
}

static void run_session(session_launch *launch)
{
    GError *error = NULL;
    char **argv = NULL;

    if (!g_shell_parse_argv(launch->command, NULL, &argv, &error)) {
        g_warning("Cannot parse the session command \"%s\": %s", launch->command, error->message);
        g_error_free(error);
        launch->exit_code = EXIT_FAILURE;
        notify_ended(launch);
        return;
    }
    if (!spawn_watched(launch, argv, on_session_exited)) {
        launch->exit_code = EXIT_FAILURE;
        notify_ended(launch);
    }
    g_strfreev(argv);
}

static bool spawn_watched(session_launch *launch, char **argv, GChildWatchFunc on_exit)
{
    GSpawnFlags flags = G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD;
    GError *error = NULL;
    GPid pid;

    if (!g_spawn_async(NULL, argv, NULL, flags, NULL, NULL, &pid, &error)) {
        g_warning("Cannot run \"%s\": %s", argv[0], error->message);
        g_error_free(error);
        return false;
    }
    start_watch(launch, g_child_watch_source_new(pid), G_SOURCE_FUNC(on_exit));
    return true;
}

static void on_session_exited(GPid pid, gint status, gpointer data)
{
    session_launch *launch = data;

    g_spawn_close_pid(pid);
    forget_watch(launch);
    launch->exit_code = exit_code_of(status);
    report_exit(launch->command, status);
    notify_ended(launch);
}

int session_launch_exit_code(const session_launch *launch)
{
    return launch->exit_code;
}

static int exit_code_of(gint status)
{
    enum { SIGNAL_EXIT_BASE = 128 };

    if (WIFSIGNALED(status)) {
        return SIGNAL_EXIT_BASE + WTERMSIG(status);
    }
    return WIFEXITED(status) ? WEXITSTATUS(status) : EXIT_FAILURE;
}

static void report_exit(const char *command, gint status)
{
    if (WIFSIGNALED(status)) {
        g_warning("The session \"%s\" was killed by signal %d", command, WTERMSIG(status));
    } else if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        g_warning("The session \"%s\" exited with status %d", command, WEXITSTATUS(status));
    }
}

static void notify_ended(session_launch *launch)
{
    if (launch->listener.ended) {
        launch->listener.ended(launch->listener.self);
    }
}
