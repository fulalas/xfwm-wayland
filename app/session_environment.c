#include "app.h"

static xfce_session_display current_display(app *app);

void app_start_session(app *app)
{
    xfce_session_display display = current_display(app);

    if (app->startup_command) {
        xfce_environment_start_session(app->xfce, &display, app->startup_command);
    } else {
        xfce_environment_export_session(app->xfce, &display);
    }
}

void app_export_x11_display(app *app)
{
    xfce_session_display display = current_display(app);

    xfce_environment_display_changed(app->xfce, &display);
}

static xfce_session_display current_display(app *app)
{
    return (xfce_session_display) {
        .wayland_display = compositor_socket_name(app->compositor),
        .x11_display = compositor_x11_display(app->compositor),
    };
}
