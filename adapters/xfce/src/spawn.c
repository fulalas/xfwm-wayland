#include "spawn.h"

#include <glib.h>

void spawn_command(const char *command)
{
    GError *error = NULL;

    if (!g_spawn_command_line_async(command, &error)) {
        g_warning("Cannot run \"%s\": %s", command, error->message);
        g_error_free(error);
    }
}
