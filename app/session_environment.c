#include "app.h"

void app_export_session_environment(app *app)
{
    xfce_session_display display = {
        .wayland_display = compositor_socket_name(app->compositor),
        .x11_display = compositor_x11_display(app->compositor),
    };

    xfce_environment_export_session(app->xfce, &display);
}

void app_run_startup_command(app *app)
{
    if (app->startup_command && *app->startup_command) {
        xfce_environment_spawn(app->xfce, app->startup_command);
    }
}
