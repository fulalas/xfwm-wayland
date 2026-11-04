#include <stdlib.h>
#include "session_launch.h"

#include <glib.h>
#include <stdbool.h>
#include <sys/wait.h>

#include "session_export.h"

struct session_launch {
    session_launch_listener listener;
    char *command;
    guint watch;
    int exit_code;
};

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
    launch->watch = g_idle_add(on_start, launch);
    return launch;
}

void session_launch_destroy(session_launch *launch)
{
    if (!launch) {
        return;
    }
    if (launch->watch) {
        g_source_remove(launch->watch);
    }
    g_free(launch->command);
    g_free(launch);
}

static gboolean on_start(gpointer data)
{
    session_launch *launch = data;

    launch->watch = 0;
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
    launch->watch = 0;
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
    launch->watch = g_child_watch_add(pid, on_exit, launch);
    return true;
}

static void on_session_exited(GPid pid, gint status, gpointer data)
{
    session_launch *launch = data;

    g_spawn_close_pid(pid);
    launch->watch = 0;
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
