#include "xwayland_startup.h"

#include <dirent.h>
#include <fcntl.h>
#include <glib.h>
#include <limits.h>
#include <signal.h>
#include <spawn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wayland-server-core.h>
#include <wlr/util/log.h>

typedef struct startup_child {
    xwayland_startup *owner;
    unsigned run;
    pid_t pid;
    struct wl_event_source *exit_source;
    struct wl_list link;
} startup_child;

struct xwayland_startup {
    struct wl_event_loop *loop;
    const char *dir;
    int hold_ms;
    struct wl_event_source *deadline;
    struct dirent **scripts;
    int script_count;
    int next_script;
    unsigned run;
    char **env;
    xwayland_startup_done done;
    void *done_data;
    struct wl_list children;
};

static void advance(xwayland_startup *startup);
static bool spawn_script(xwayland_startup *startup, const char *path);
static void init_spawn_attr(posix_spawnattr_t *attr);
static bool is_runnable(const char *path);
static int skip_hidden(const struct dirent *entry);
static bool watch_child(xwayland_startup *startup, pid_t pid);
static int handle_child_exit(int fd, uint32_t mask, void *data);
static int handle_deadline(void *data);
static void free_scripts(xwayland_startup *startup);
static void finish_hold(xwayland_startup *startup);
static void drop_child(startup_child *child);
static void stop_unwatched(pid_t pid);

xwayland_startup *xwayland_startup_create(struct wl_event_loop *loop, const char *dir,
                                          int hold_ms)
{
    xwayland_startup *startup = calloc(1, sizeof(*startup));
    if (startup == NULL) {
        return NULL;
    }
    startup->loop = loop;
    startup->dir = dir;
    startup->hold_ms = hold_ms;
    startup->deadline = wl_event_loop_add_timer(loop, handle_deadline, startup);
    wl_list_init(&startup->children);
    if (startup->deadline == NULL) {
        free(startup);
        return NULL;
    }
    return startup;
}

void xwayland_startup_destroy(xwayland_startup *startup)
{
    if (startup == NULL) {
        return;
    }
    xwayland_startup_cancel(startup);
    startup_child *child;
    startup_child *next;
    wl_list_for_each_safe(child, next, &startup->children, link) {
        drop_child(child);
    }
    wl_event_source_remove(startup->deadline);
    free(startup);
}

void xwayland_startup_run(xwayland_startup *startup, const char *display,
                          xwayland_startup_done done, void *data)
{
    xwayland_startup_cancel(startup);
    startup->env = g_environ_setenv(g_get_environ(), "DISPLAY", display, TRUE);
    startup->done = done;
    startup->done_data = data;
    int count = scandir(startup->dir, &startup->scripts, skip_hidden, alphasort);
    startup->script_count = count > 0 ? count : 0;
    if (done != NULL && startup->script_count > 0) {
        wl_event_source_timer_update(startup->deadline, startup->hold_ms);
    }
    advance(startup);
}

void xwayland_startup_cancel(xwayland_startup *startup)
{
    startup_child *child;
    wl_list_for_each(child, &startup->children, link) {
        if (child->run == startup->run) {
            kill(-child->pid, SIGTERM);
            kill(-child->pid, SIGCONT);
        }
    }
    startup->run++;
    free_scripts(startup);
    startup->done = NULL;
    finish_hold(startup);
}

static void advance(xwayland_startup *startup)
{
    char path[PATH_MAX];
    while (startup->next_script < startup->script_count) {
        const struct dirent *entry = startup->scripts[startup->next_script++];
        snprintf(path, sizeof(path), "%s/%s", startup->dir, entry->d_name);
        if (is_runnable(path) && spawn_script(startup, path)) {
            return;
        }
    }
    free_scripts(startup);
    finish_hold(startup);
}

static bool spawn_script(xwayland_startup *startup, const char *path)
{
    posix_spawnattr_t attr;
    init_spawn_attr(&attr);
    posix_spawn_file_actions_t files;
    posix_spawn_file_actions_init(&files);
    posix_spawn_file_actions_addopen(&files, STDIN_FILENO, "/dev/null", O_RDONLY, 0);
    pid_t pid = 0;
    char *argv[] = {(char *) path, NULL};
    int failed = posix_spawn(&pid, path, &files, &attr, argv, startup->env);
    posix_spawn_file_actions_destroy(&files);
    posix_spawnattr_destroy(&attr);
    if (failed != 0) {
        wlr_log(WLR_ERROR, "cannot run X11 startup script %s: %s", path, strerror(failed));
        return false;
    }
    return watch_child(startup, pid);
}

static void init_spawn_attr(posix_spawnattr_t *attr)
{
    posix_spawnattr_init(attr);
    sigset_t signals;
    sigemptyset(&signals);
    posix_spawnattr_setsigmask(attr, &signals);
    sigaddset(&signals, SIGPIPE);
    sigaddset(&signals, SIGCHLD);
    posix_spawnattr_setsigdefault(attr, &signals);
    posix_spawnattr_setpgroup(attr, 0);
    posix_spawnattr_setflags(attr, POSIX_SPAWN_SETSIGMASK | POSIX_SPAWN_SETSIGDEF |
                                       POSIX_SPAWN_SETPGROUP);
}

static bool is_runnable(const char *path)
{
    struct stat info;
    return stat(path, &info) == 0 && S_ISREG(info.st_mode) && access(path, X_OK) == 0;
}

static int skip_hidden(const struct dirent *entry)
{
    return entry->d_name[0] != '.';
}

static bool watch_child(xwayland_startup *startup, pid_t pid)
{
    int fd = (int) syscall(SYS_pidfd_open, pid, 0);
    if (fd < 0) {
        wlr_log(WLR_ERROR, "cannot wait for X11 startup script %d", (int) pid);
        stop_unwatched(pid);
        return false;
    }
    startup_child *child = malloc(sizeof(*child));
    if (child != NULL) {
        *child = (startup_child){.owner = startup, .run = startup->run, .pid = pid};
        child->exit_source = wl_event_loop_add_fd(startup->loop, fd, WL_EVENT_READABLE,
                                                  handle_child_exit, child);
    }
    close(fd);
    if (child == NULL || child->exit_source == NULL) {
        free(child);
        stop_unwatched(pid);
        return false;
    }
    wl_list_insert(&startup->children, &child->link);
    return true;
}

static int handle_child_exit(int fd, uint32_t mask, void *data)
{
    (void) fd;
    (void) mask;
    startup_child *child = data;
    xwayland_startup *startup = child->owner;
    bool current = child->run == startup->run;
    waitpid(child->pid, NULL, WNOHANG);
    drop_child(child);
    if (current) {
        advance(startup);
    }
    return 0;
}

static int handle_deadline(void *data)
{
    xwayland_startup *startup = data;
    wlr_log(WLR_ERROR, "X11 startup scripts still running, letting X11 clients in");
    finish_hold(startup);
    return 0;
}

static void free_scripts(xwayland_startup *startup)
{
    for (int i = 0; i < startup->script_count; i++) {
        free(startup->scripts[i]);
    }
    free(startup->scripts);
    startup->scripts = NULL;
    startup->script_count = 0;
    startup->next_script = 0;
    g_strfreev(startup->env);
    startup->env = NULL;
}

static void finish_hold(xwayland_startup *startup)
{
    wl_event_source_timer_update(startup->deadline, 0);
    xwayland_startup_done done = startup->done;
    void *data = startup->done_data;
    startup->done = NULL;
    startup->done_data = NULL;
    if (done != NULL) {
        done(data);
    }
}

static void drop_child(startup_child *child)
{
    wl_event_source_remove(child->exit_source);
    wl_list_remove(&child->link);
    free(child);
}

static void stop_unwatched(pid_t pid)
{
    kill(-pid, SIGKILL);
    waitpid(pid, NULL, 0);
}
