#include "session_export.h"

#include <glib.h>

#include "spawn.h"

static const char *exported_variables[] = {
    "WAYLAND_DISPLAY", "DISPLAY", "XDG_CURRENT_DESKTOP", "XDG_SESSION_TYPE", NULL,
};

static void set_display_variables(const xfce_session_display *display);
static void update_activation_environment(void);

void session_export(const xfce_session_display *display)
{
    set_display_variables(display);
    g_setenv("XDG_SESSION_TYPE", "wayland", TRUE);
    if (!g_getenv("XDG_CURRENT_DESKTOP")) {
        g_setenv("XDG_CURRENT_DESKTOP", "XFCE", TRUE);
    }
    update_activation_environment();
}

static void set_display_variables(const xfce_session_display *display)
{
    g_setenv("WAYLAND_DISPLAY", display->wayland_display, TRUE);
    if (display->x11_display) {
        g_setenv("DISPLAY", display->x11_display, TRUE);
    }
}

static void update_activation_environment(void)
{
    gchar *tool = g_find_program_in_path("dbus-update-activation-environment");
    gchar *variables;
    gchar *command;

    if (!tool) {
        return;
    }
    variables = g_strjoinv(" ", (gchar **) exported_variables);
    command = g_strdup_printf("%s --systemd %s", tool, variables);
    spawn_command(command);
    g_free(command);
    g_free(variables);
    g_free(tool);
}
