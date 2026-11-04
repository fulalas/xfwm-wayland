#include <glib.h>
#include <signal.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include "app.h"

static bool use_existing_bus(void);
static bool use_runtime_bus(void);
static GPid spawn_private_bus(void);
static bool export_address(int output_fd);

GPid app_session_bus_start(void)
{
    if (use_existing_bus()) {
        return 0;
    }
    return spawn_private_bus();
}

void app_session_bus_stop(GPid bus)
{
    if (bus > 0) {
        kill(bus, SIGTERM);
        g_spawn_close_pid(bus);
    }
}

static bool use_existing_bus(void)
{
    const char *address = g_getenv("DBUS_SESSION_BUS_ADDRESS");

    return (address && *address) || use_runtime_bus();
}

static bool use_runtime_bus(void)
{
    const char *runtime = g_getenv("XDG_RUNTIME_DIR");
    gchar *path = runtime ? g_build_filename(runtime, "bus", NULL) : NULL;
    struct stat info;
    bool present = path && stat(path, &info) == 0 && S_ISSOCK(info.st_mode);

    if (present) {
        gchar *address = g_strdup_printf("unix:path=%s", path);
        g_setenv("DBUS_SESSION_BUS_ADDRESS", address, TRUE);
        g_free(address);
    }
    g_free(path);
    return present;
}

static GPid spawn_private_bus(void)
{
    char *argv[] = {"dbus-daemon", "--session", "--nofork", "--print-address=1", NULL};
    GError *error = NULL;
    GPid pid = 0;
    int output_fd = -1;

    if (!g_spawn_async_with_pipes(NULL, argv, NULL, G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
                                  NULL, NULL, &pid, NULL, &output_fd, NULL, &error)) {
        g_warning("Cannot start a D-Bus session bus: %s", error->message);
        g_error_free(error);
        return 0;
    }
    if (!export_address(output_fd)) {
        g_warning("The D-Bus session bus did not report its address");
        app_session_bus_stop(pid);
        return 0;
    }
    return pid;
}

static bool export_address(int output_fd)
{
    char line[512] = "";
    FILE *output = fdopen(output_fd, "r");
    bool read_ok = output && fgets(line, sizeof(line), output) != NULL;

    if (output) {
        fclose(output);
    }
    g_strchomp(line);
    if (!read_ok || line[0] == '\0') {
        return false;
    }
    g_setenv("DBUS_SESSION_BUS_ADDRESS", line, TRUE);
    return true;
}
