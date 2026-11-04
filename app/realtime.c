#include "app.h"

void app_realtime_start(void)
{
    gchar *argv[] = {(gchar *) APP_REALTIME_HELPER, NULL};
    gint status = 0;
    GError *error = NULL;
    gboolean spawned = g_spawn_sync(NULL, argv, NULL,
                                    G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL,
                                    NULL, NULL, NULL, NULL, &status, &error);
    if (!spawned || !g_spawn_check_wait_status(status, NULL)) {
        g_message("running without realtime priority");
    }
    g_clear_error(&error);
}
