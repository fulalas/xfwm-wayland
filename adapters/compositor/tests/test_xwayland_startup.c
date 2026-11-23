#include <ftw.h>
#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <wayland-server-core.h>

#include "check.h"
#include "util/clock.h"
#include "xwayland/xwayland_startup.h"

enum {
    HOLD_MS = 300,
    WAIT_MS = 3000,
};

typedef struct fixture {
    char *dir;
    char *log;
    struct wl_event_loop *loop;
    xwayland_startup *startup;
    int done_count;
} fixture;

static void setup_in(fixture *f, const char *dir)
{
    f->dir = g_dir_make_tmp("xwayland-startup-XXXXXX", NULL);
    CHECK(f->dir != NULL);
    f->log = g_build_filename(f->dir, ".log", NULL);
    f->loop = wl_event_loop_create();
    f->startup = xwayland_startup_create(f->loop, dir != NULL ? dir : f->dir, HOLD_MS);
    f->done_count = 0;
}

static void setup(fixture *f)
{
    setup_in(f, NULL);
}

static int remove_entry(const char *path, const struct stat *st, int flag, struct FTW *ftw)
{
    (void) st;
    (void) flag;
    (void) ftw;
    return remove(path);
}

static void teardown(fixture *f)
{
    xwayland_startup_destroy(f->startup);
    wl_event_loop_destroy(f->loop);
    nftw(f->dir, remove_entry, 8, FTW_DEPTH | FTW_PHYS);
    g_free(f->log);
    g_free(f->dir);
}

static void add_script(const fixture *f, const char *name, const char *body, mode_t mode)
{
    char *path = g_build_filename(f->dir, name, NULL);
    char *text = g_strdup_printf("#!/bin/sh\n%s\n", body);
    CHECK(g_file_set_contents(path, text, -1, NULL));
    CHECK(chmod(path, mode) == 0);
    g_free(text);
    g_free(path);
}

static void add_logging_script(const fixture *f, const char *name, const char *extra)
{
    char *body = g_strdup_printf("echo \"%s $DISPLAY\" >> '%s'\n%s", name, f->log, extra);
    add_script(f, name, body, 0755);
    g_free(body);
}

static void count_done(void *data)
{
    fixture *f = data;
    f->done_count++;
}

static char *read_log(const fixture *f)
{
    char *text = NULL;
    if (!g_file_get_contents(f->log, &text, NULL, NULL)) {
        return g_strdup("");
    }
    return text;
}

static bool log_has(const fixture *f, const char *line)
{
    char *log = read_log(f);
    bool found = strstr(log, line) != NULL;
    g_free(log);
    return found;
}

static int64_t dispatch_until(fixture *f, const char *line, int64_t limit_ms)
{
    int64_t start = clock_now_ms();
    while (clock_now_ms() - start < limit_ms &&
           (line != NULL ? !log_has(f, line) : f->done_count == 0)) {
        wl_event_loop_dispatch(f->loop, 20);
    }
    return clock_now_ms() - start;
}

static int64_t run_until_done(fixture *f, const char *display)
{
    xwayland_startup_run(f->startup, display, count_done, f);
    return dispatch_until(f, NULL, WAIT_MS);
}

static void test_missing_folder_releases_at_once(void)
{
    fixture f;
    setup_in(&f, "/nonexistent/startup.d");
    xwayland_startup_run(f.startup, ":5", count_done, &f);
    CHECK_INT_EQ(1, f.done_count);
    teardown(&f);
}

static void test_runs_scripts_in_name_order_with_display(void)
{
    fixture f;
    setup(&f);
    add_logging_script(&f, "20-second", "");
    add_logging_script(&f, "10-first", "");
    run_until_done(&f, ":7");
    char *log = read_log(&f);
    CHECK_INT_EQ(1, f.done_count);
    CHECK(strcmp("10-first :7\n20-second :7\n", log) == 0);
    g_free(log);
    teardown(&f);
}

static void test_scripts_read_nothing(void)
{
    fixture f;
    setup(&f);
    char *body = g_strdup_printf("cat > /dev/null\necho \"read $DISPLAY\" >> '%s'", f.log);
    add_script(&f, "10-reads", body, 0755);
    g_free(body);
    run_until_done(&f, ":6");
    CHECK(log_has(&f, "read :6\n"));
    teardown(&f);
}

static void test_skips_hidden_and_non_executable_files(void)
{
    fixture f;
    setup(&f);
    add_logging_script(&f, "10-run", "");
    add_script(&f, "20-plain", "echo plain >> /dev/null", 0644);
    add_script(&f, ".30-hidden", "exit 0", 0755);
    run_until_done(&f, ":1");
    char *log = read_log(&f);
    CHECK_INT_EQ(1, f.done_count);
    CHECK(strcmp("10-run :1\n", log) == 0);
    g_free(log);
    teardown(&f);
}

static void test_failing_script_does_not_stop_the_rest(void)
{
    fixture f;
    setup(&f);
    add_logging_script(&f, "10-fails", "exit 3");
    add_logging_script(&f, "20-runs", "");
    run_until_done(&f, ":2");
    char *log = read_log(&f);
    CHECK_INT_EQ(1, f.done_count);
    CHECK(strcmp("10-fails :2\n20-runs :2\n", log) == 0);
    g_free(log);
    teardown(&f);
}

static void test_hanging_script_releases_after_the_hold(void)
{
    fixture f;
    setup(&f);
    add_logging_script(&f, "10-slow", "sleep 1");
    add_logging_script(&f, "20-after", "");
    int64_t ms = run_until_done(&f, ":3");
    CHECK_INT_EQ(1, f.done_count);
    CHECK(ms >= HOLD_MS);
    CHECK(!log_has(&f, "20-after"));
    dispatch_until(&f, "20-after :3\n", WAIT_MS);
    char *log = read_log(&f);
    CHECK(strcmp("10-slow :3\n20-after :3\n", log) == 0);
    CHECK_INT_EQ(1, f.done_count);
    g_free(log);
    teardown(&f);
}

static void test_new_run_stops_the_old_one(void)
{
    fixture f;
    setup(&f);
    char *slow = g_strdup_printf("(sleep 0.2; echo \"after $DISPLAY\" >> '%s') &\nwait", f.log);
    add_logging_script(&f, "10-slow", slow);
    g_free(slow);
    add_logging_script(&f, "20-next", "");
    xwayland_startup_run(f.startup, ":8", count_done, &f);
    dispatch_until(&f, "10-slow :8\n", WAIT_MS);
    run_until_done(&f, ":9");
    dispatch_until(&f, "after :8", 300);
    char *log = read_log(&f);
    CHECK_INT_EQ(1, f.done_count);
    CHECK(strstr(log, "10-slow :8\n") != NULL);
    CHECK(strstr(log, "after :9\n") != NULL);
    CHECK(strstr(log, "20-next :9\n") != NULL);
    CHECK(strstr(log, "after :8") == NULL);
    CHECK(strstr(log, "20-next :8") == NULL);
    g_free(log);
    teardown(&f);
}

int main(void)
{
    const check_case cases[] = {
        {"missing_folder_releases_at_once", test_missing_folder_releases_at_once},
        {"runs_scripts_in_name_order_with_display", test_runs_scripts_in_name_order_with_display},
        {"scripts_read_nothing", test_scripts_read_nothing},
        {"skips_hidden_and_non_executable_files", test_skips_hidden_and_non_executable_files},
        {"failing_script_does_not_stop_the_rest", test_failing_script_does_not_stop_the_rest},
        {"hanging_script_releases_after_the_hold", test_hanging_script_releases_after_the_hold},
        {"new_run_stops_the_old_one", test_new_run_stops_the_old_one},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
