#include "session_export.h"

#include <glib.h>

#include "session_vars.h"

static const char activation_tool[] = "dbus-update-activation-environment";

static void set_display_variables(const xfce_session_display *display);
static void set_missing_defaults(void);
static const char *lookup_variable(void *self, const char *name);

void session_export(const xfce_session_display *display)
{
    set_display_variables(display);
    g_setenv("XDG_SESSION_TYPE", "wayland", TRUE);
    set_missing_defaults();
}

char **session_export_activation_argv(void)
{
    session_lookup lookup = { .self = NULL, .get = lookup_variable };
    gchar *tool = g_find_program_in_path(activation_tool);
    char **argv;

    if (!tool) {
        return NULL;
    }
    argv = session_vars_activation_argv(tool, &lookup);
    g_free(tool);
    return argv;
}

void session_export_update_activation(void)
{
    char **argv = session_export_activation_argv();
    GError *error = NULL;

    if (argv && !g_spawn_async(NULL, argv, NULL, G_SPAWN_DEFAULT, NULL, NULL, NULL, &error)) {
        g_warning("Cannot update the D-Bus environment: %s", error->message);
        g_error_free(error);
    }
    g_strfreev(argv);
}

static void set_display_variables(const xfce_session_display *display)
{
    g_setenv("WAYLAND_DISPLAY", display->wayland_display, TRUE);
    if (display->x11_display) {
        g_setenv("DISPLAY", display->x11_display, TRUE);
    } else {
        g_unsetenv("DISPLAY");
    }
}

static void set_missing_defaults(void)
{
    for (size_t i = 0; session_vars_defaults[i].name; i++) {
        g_setenv(session_vars_defaults[i].name, session_vars_defaults[i].value, FALSE);
    }
}

static const char *lookup_variable(void *self, const char *name)
{
    (void) self;
    return g_getenv(name);
}
