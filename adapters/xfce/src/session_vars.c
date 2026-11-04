#include "session_vars.h"

#include <glib.h>
#include <stdbool.h>

const session_variable session_vars_defaults[] = {
    { "XDG_CURRENT_DESKTOP", "XFCE" },
    { "XDG_SESSION_DESKTOP", "xfce" },
    { "DESKTOP_SESSION", "xfce" },
    { "XDG_MENU_PREFIX", "xfce-" },
    { NULL, NULL },
};

static const char *const exported_names[] = {
    "WAYLAND_DISPLAY",
    "DISPLAY",
    "XDG_SESSION_TYPE",
    "XDG_CURRENT_DESKTOP",
    "XDG_SESSION_DESKTOP",
    "DESKTOP_SESSION",
    "XDG_MENU_PREFIX",
    "XDG_CONFIG_DIRS",
    "XDG_DATA_DIRS",
    NULL,
};

static const char systemd_option[] = "--systemd";

static bool is_set(const session_lookup *lookup, const char *name);

char **session_vars_activation_argv(const char *tool, const session_lookup *lookup)
{
    GPtrArray *argv = g_ptr_array_new();

    g_ptr_array_add(argv, g_strdup(tool));
    g_ptr_array_add(argv, g_strdup(systemd_option));
    for (size_t i = 0; exported_names[i]; i++) {
        if (is_set(lookup, exported_names[i])) {
            g_ptr_array_add(argv, g_strdup(exported_names[i]));
        }
    }
    g_ptr_array_add(argv, NULL);
    return (char **) g_ptr_array_free(argv, FALSE);
}

static bool is_set(const session_lookup *lookup, const char *name)
{
    const char *value = lookup->get(lookup->self, name);

    return value && *value;
}
